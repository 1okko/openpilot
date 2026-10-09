#pragma once

// I 帧/丢帧状态机（16 号 (b)）：纯逻辑事件输出，调用方执行 request_keyframe()。
//
// 口径（README「实现口径」）：
//   - 两路基准 GOP 20（编码器配置 NUM_P_FRAMES=19），wide 相位比 road 晚 10 帧
//     （road IDR = seq 0,20,40,…；wide IDR = seq 0,10,30,50,…，与 12 号素材同分布）。
//   - 每条连接起始、每次丢帧后：两路 request_keyframe() —— request 必须先于下一个
//     提交帧的 encode_frame（21 号实测「下一帧立即 I 帧」），故事件在提交时刻之前发出，
//     落点 = 事件后的第一个提交帧 = 新序列第 0 帧。
//   - 新序列第 10 帧给 wide 再补一次 request_keyframe()（恢复「wide 晚 10 帧」相位）。
//   - 只有码流断档才触发丢帧恢复：发送侧显式丢弃（kDrop：队列覆盖、段长超限、
//     主线上报编码输出缺帧，走 on_frame_dropped 按 frame_idx 去重）与 MetaCache
//     kGapMiss（情形 C：输出被吞，走 on_stream_gap 不去重）。配对失败和
//     frame_idx 时间槽空洞都只记账，不重开序列。假死截断/重连仍由 kNewConnection
//     开新序列。序列头门丢弃（kHeadBarrier）非断档，但同样经 on_frame_dropped 起
//     新序列 + 双路 request（断档后首帧恒为双路 IDR 对）。
//   - IDR 预测（FRAME 头 flags）：bit0 = road 段 IDR（调用方再与实际 keyframe 位与）、
//     bit1 = wide 段 IDR（预测值；服务端 hevc_decoder 以 NAL 实读为准，flags 只做
//     「置位必须为真」的一致性检查，故预测只许欠报不许误报——request 落点即保证）。
//
// frame_idx 是 road timestamp_sof 对应的 50 ms 时间槽编号；每条连接从 0 起，空槽是
// 正常时间间隔，不是码流断档。基准由调用方在 kNewConnection 事件后重置。

#include <cstdint>

#include "frame_ids.h"

// 将 road timestamp_sof 映射到 50 ms 时间槽：首个被编号 SOF 为基准，连接内严格递增。
class FrameIndexer {
 public:
  static constexpr uint64_t kFramePeriodNs = 50'000'000ULL;

  void reset();
  FrameIdx index(uint64_t road_sof_ns);

 private:
  bool have_base_ = false;
  uint64_t sof_base_ns_ = 0;
  uint64_t last_idx_ = 0;
};

struct SchedStep {
  bool request_keyframe_road = false;
  bool request_keyframe_wide = false;
  bool drop_detected = false;  // 本次输入触发丢帧语义（新序列开始）
  bool road_idr = false;       // 仅 on_frame_submit 填：本帧 IDR 预测（FRAME 头 flags）
  bool wide_idr = false;

  bool any() const {
    return request_keyframe_road || request_keyframe_wide || drop_detected || road_idr || wide_idr;
  }
};

class FrameScheduler {
 public:
  // 连接建立/重连成功（kNewConnection）：新序列，两路 request（落在下一个提交帧）。
  SchedStep on_connect();

  // 帧提交编码前调用（事件必须先于本帧 encode_frame）。seq 位置 10 给 wide 补 I，
  // 返回本帧 IDR 预测；frame_idx 空槽不触发恢复。
  SchedStep on_frame_submit(FrameIdx frame_idx);

  // 码流断档（有 frame_idx 键的显式上报）：立即新序列；调用方两种情形——发送侧
  // 显式丢弃（kDrop）、序列头门丢弃（kHeadBarrier）。配对失败/时间槽空洞不调用此接口。
  // 同一 frame 的重复显式上报只报一次（例如同一帧的两路编码段都超限）。
  SchedStep on_frame_dropped(FrameIdx frame_idx);

  // 码流断档（无键上报：MetaCache kGapMiss，情形 C）：不参与 frame_idx 去重
  // （main.cc 合成，跨路 frame_id 不入 frame_idx 键空间），每次都开新序列。
  SchedStep on_stream_gap();

 private:
  SchedStep start_new_sequence(bool drop_detected);
  bool drop_already_reported(FrameIdx frame_idx);

  int seq_pos_ = 0;
  bool pending_req_road_ = false;
  bool pending_req_wide_ = false;
  int next_idr_seq_road_ = 0;
  int next_idr_seq_wide_ = 0;

  // 同一 frame 重复上报去重（发送侧同一帧只报一次）：每连接内最近
  // kDropDedupWindow 次显式丢弃的 idx 窗口；on_connect 清空（frame_idx 重新
  // 编号）。（18 号 #5 后配对死亡只记账、不上报 scheduler，此处只服务发送侧。）
  static constexpr int kDropDedupWindow = 32;
  FrameIdx recent_drop_[kDropDedupWindow] = {};
  int recent_drop_n_ = 0;
  int recent_drop_pos_ = 0;
};
