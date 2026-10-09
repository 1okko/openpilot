#pragma once

// 组帧提交上下文查表（18 号 #4+#5）：每编码器一条 FIFO（同路编码输出按提交序返回）。
//
// 根因（2026-09-28 真机 clean2 轮）：旧版 take()「查不到就弹掉整个队列」——重连
// meta.clear() 后旧连接的编码输出迟到查不到键，把新序列的条目一起吃掉，序列头
// 双路 IDR 对整对消失（PyAV decoded < recv、I 帧落点整体偏一位）。
// 修法：**查不到只分类报告、绝不弹队**；命中只弹匹配项及其前面的死条目。
//
// miss 分类（调用方据此决定是否上报码流断档）：
//   kStaleMiss：旧连接/已清/已弹的迟到输出（连接建立窗口）——静默丢弃，序列头门
//     兜底，不许误报成断档。
//   kGapMiss：本应在队却查无（输出被吞）＝ 码流断档（情形 C）——必须上报调度器
//     （新序列 + 双路 request）并让发送侧序列头门重新关门。
//   命中前缀的死条目（FIFO 下其输出永不会来）同样是断档，随命中一并返回。
//
// 纯逻辑（仅 std），宿主单测直接编译（test_bigmodeld.cc）。

#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "frame_codec.h"
#include "frame_ids.h"

// 编码输出回调按 frame_id 取回的提交上下文
struct OutMeta {
  CamFrameId frame_id{};    // 查表键 = 各路自己的 frame_id（两路计数器独立、持续漂移）
  FrameIdx frame_idx{};     // 连接内 50 ms 时间槽编号（进 FRAME 头）
  ConnEpoch conn_epoch{};   // 连接代号（sender 建连次数）：迟到旧连接输出在 submit 侧拒绝
  bool road_idr_pred = false;  // 本帧 IDR 预测（FRAME 头 bit0 用实际位，此处供序列头门）
  bool wide_idr_pred = false;  // FRAME 头 bit1（预测；只许欠报不许误报）
  bgm1::FrameHeader hdr;
};

enum class MetaTake {
  kHit,        // 命中：om 有效；dead 带回前缀死条目（其输出已丢 = 断档）
  kStaleMiss,  // 迟到旧输出（连接窗口/已清/已弹）：静默丢弃
  kGapMiss,    // 查无且非旧：输出被吞 = 码流断档，上报
};

class MetaCache {
 public:
  struct Result {
    MetaTake st = MetaTake::kStaleMiss;
    OutMeta om;               // kHit 时有效
    std::vector<OutMeta> dead;  // kHit 时：被跳过的前缀死条目（FIFO 下输出已丢）
  };

  void push(const OutMeta& m) {
    std::lock_guard<std::mutex> lk(mtx_);
    q_.push_back(m);
    pushed_since_clear_ = true;
    last_pushed_id_ = m.frame_id;
  }

  Result take(CamFrameId frame_id) {
    std::lock_guard<std::mutex> lk(mtx_);
    Result r;
    for (size_t i = 0; i < q_.size(); i++) {
      if (q_[i].frame_id != frame_id) continue;
      r.st = MetaTake::kHit;
      r.om = q_[i];
      for (size_t j = 0; j < i; j++) r.dead.push_back(q_[j]);  // FIFO：其输出永不会来
      q_.erase(q_.begin(), q_.begin() + (long)i + 1);
      return r;
    }
    // 查不到：不弹队（关键修复——不能误吃幸存条目），只分类
    if (!pushed_since_clear_) {
      r.st = MetaTake::kStaleMiss;                       // clear 后尚无新提交：只能是旧输出
    } else if (!q_.empty() && frame_id < q_.front().frame_id) {
      r.st = MetaTake::kStaleMiss;                       // 比存活条目更旧：已清/已弹的迟到输出
    } else if (q_.empty() && frame_id <= last_pushed_id_) {
      r.st = MetaTake::kStaleMiss;
    } else {
      r.st = MetaTake::kGapMiss;                         // 查无且非旧 = 断档
    }
    return r;
  }

  // 重连清空：旧连接的编码在途输出不带进新连接（其迟到输出判 kStaleMiss）
  void clear() {
    std::lock_guard<std::mutex> lk(mtx_);
    q_.clear();
    pushed_since_clear_ = false;
    last_pushed_id_ = CamFrameId{};
  }

 private:
  std::mutex mtx_;
  std::deque<OutMeta> q_;
  bool pushed_since_clear_ = false;
  CamFrameId last_pushed_id_{};
};
