#pragma once

// 发送/重连状态机（16 号 (c)）+ REPLY 接收记账（(e)）。纯逻辑 + 可注入 socket/时钟，
// 宿主单测用假 socket/假时钟（test_bigmodeld.cc）。
//
// 口径（README「实现口径」）：
//   - TCP NODELAY；road 出包即发：submit_road 后 chunk1（头 160 B ‖ road 段）即可发，
//     chunk2（wide_len ‖ wide ‖ MAC 16 B）等 wide 包到齐再发（10 号布局分块发送，
//     与 fake_c4_client.py 同款）。**序列头门关门期间例外**：整对扣在排队槽，
//     双路 IDR 实测齐全才开流（见下）。
//   - 每路一个在途槽 + 一个排队槽：在途帧（正在发送）发完不撕裂；排队槽只留最新，
//     新包覆盖排队槽 = 丢弃旧包（kDrop 上报「丢帧」）；旧 frame_idx 的迟到包与
//     已丢帧的残包静默丢弃（丢帧已随覆盖/屏障上报）。
//   - **序列头门（18 号 #1/#2）**：连接建立、截断重连、码流断档（kDrop / 主线上报
//     notify_stream_gap）之后，第一个发出的帧必须是「预测双 IDR + 实测双 IDR」对；
//     不合格对整对丢弃（kHeadBarrier：非断档，调用方上报新序列 + 补双路 request），
//     直到合格对上线才开门。断档后首帧/连接首帧恒为双路 IDR 对，服务端判据可
//     简化为「中流双路 IDR = 新序列」。kDrop 丢旧包连替代帧一起按此把关（#1：
//     替代帧若是 P 帧不得成为断档后首帧）。
//   - **连接代号（18 号 #2）**：submit_* 携带调用方的 conn_epoch（kNewConnection
//     事件里带出），与本连接代号不符的迟到提交静默丢弃——
//     重连窗口内旧连接的编码在途输出不得混进新连接（旧 frame_idx 会污染编号）。
//   - 假死 = 对在途帧 ≥200 ms 无任何进展（socket 写阻塞，或 wide 段迟迟不到）：
//     尝试发完在途帧（写得动就发完，不撕裂），否则截断，随后重连，上报 kStall。
//     （wide 段死亡 = 线协议撕裂，只有截断重连一条路。）
//   - 连接断开/发送失败重连：每次 connect 1 s 超时；连续 3 次失败升级 kLinkLost
//     （「连接丢失」），此后持续重试、成功即复位；建连/重连成功报 kNewConnection
//     （调用方重置 frame_idx 与 I 帧状态）。重连清空在途/排队（旧帧不带进新连接）。
//   - wide IDR 预测（FRAME 头 bit1）假设：request_keyframe 先于目标帧 encode_frame
//     发出即落在该帧（21 号实测「下一帧立即 I 帧」），V4L 输入队列无积压时成立；
//     序列头帧由本门实测双 IDR 验证（误报不可能出在头帧），中流误报由
//     bm16_check「flags ↔ 码流一致」实测兜住。
//
// 线程模型：submit_road/submit_wide 由编码回调线程调，step() 由发送线程调，
// notify_disconnect()/notify_stream_gap() 由编码回调/REPLY 线程调；内部互斥。
// 事件回调在锁内触发（调用方不得回调进本类）。

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "frame_codec.h"
#include "frame_ids.h"

// 可注入系统接缝（宿主测试用假 socket）
class UplinkSocket {
 public:
  virtual ~UplinkSocket() = default;
  virtual bool connect(int timeout_ms) = 0;                            // 建连；超时/失败 false
  virtual int write_some(const uint8_t* data, size_t len) = 0;         // >0 已写 / 0 会阻塞 / -1 错误
  virtual int read_some(uint8_t* data, size_t len) = 0;                // >0 已读 / 0 无数据 / -1 错误或 EOF
  virtual void close() = 0;
};

enum class UplinkEvent {
  kNewConnection,  // 连接建立/重连成功：调用方重置 frame_idx 与 I 帧状态（scheduler.on_connect）
  kStall,          // 假死（≥200 ms 无进展）：发完在途帧或截断后重连，上报重置
  kLinkLost,       // 连续 3 次重连失败（每次 1 s 超时）升级「连接丢失」
  kDrop,           // 排队旧帧被覆盖丢弃 / 主线上报编码输出缺帧·段长超限（码流断档）
  kStreamGap,      // 编码输出被吞（MetaCache kGapMiss 情形 C）——仅 main.cc 合成，sender 不发
  kHeadBarrier,    // 序列头门丢弃（非双路 IDR 帧不开流；非断档，调用方补 request）
  kFrameSent,      // 一帧完整发出
};

struct UplinkEventInfo {
  UplinkEvent ev;
  FrameIdx frame_idx{};
  ConnEpoch conn_epoch{};  // 仅 kNewConnection 填：连接代号，调用方 stamp 后续提交
};

struct UplinkSenderConfig {
  uint64_t stall_ms = 200;         // 假死阈值
  int connect_timeout_ms = 1000;   // 每次重连超时
  int max_connect_failures = 3;    // 连续失败升级「连接丢失」
};

class UplinkSender {
 public:
  using EventFn = std::function<void(const UplinkEventInfo&)>;

  UplinkSender(UplinkSocket* sock, std::function<uint64_t()> now_ms,
               UplinkSenderConfig cfg, EventFn on_event);

  // road 包出包即调（编码回调）。hdr 填好 t_eof/desire/traffic_convention/action_t/warp_*
  //（MetaProvider::fill）；flags 由本函数按 road_idr_actual（bit0）与 wide_idr_predicted
  //（bit1）置位——bit0 用实际 keyframe 位（精确）、bit1 用 I 帧策略预测（头先于 wide 包发出）。
  // road_idr_predicted 只供序列头门用（不进线协议）。conn_epoch 与当前连接不符时静默丢弃。
  void submit_road(ConnEpoch conn_epoch, FrameIdx frame_idx, const bgm1::FrameHeader& hdr,
                   const uint8_t* road, size_t road_len,
                   bool road_idr_actual, bool road_idr_predicted, bool wide_idr_predicted);
  // wide 包到达时调（编码回调）。
  void submit_wide(ConnEpoch conn_epoch, FrameIdx frame_idx, const uint8_t* wide, size_t wide_len,
                   bool wide_actual_idr);

  // 发送循环单步（发送线程）；返回本次是否有进展（无进展时调用方可以小睡）。
  bool step();

  // step() 无进展时发送线程在此等：有 submit_* 即返回 true，否则 timeout_ms 后 false
  //（超时兜底 socket 写阻塞/假死计时这类无提交的推进）。
  bool wait_for_work(int timeout_ms);

  // 读侧发现断连/EOF/ERR（REPLY 接收线程）→ 走重连路径。
  void notify_disconnect();

  // 码流断档（编码输出缺帧/段长超限，编码回调线程）→ 序列头门重新关门：
  // 之后第一个发出的帧必须是双路 IDR 对（断档后首帧恒为 IDR）。
  void notify_stream_gap();

  // ---- 状态查询（统计/测试）----
  bool connected() const;
  ConnEpoch conn_epoch() const;      // 当前连接代号（kNewConnection 事件同值）
 private:
  struct OutFrame {
    FrameIdx frame_idx{};
    bool has_road = false;
    bool has_wide = false;
    bool road_idr_actual = false;
    bool road_idr_predicted = false;
    bool wide_idr_predicted = false;
    bool wide_idr_actual = false;
    std::vector<uint8_t> chunk1;  // 头 160 B ‖ road 段
    std::vector<uint8_t> chunk2;  // wide_len u32 ‖ wide 段 ‖ MAC 16 B（零）
    size_t sent1 = 0;
    size_t sent2 = 0;
    bool chunk1_done = false;
  };

  OutFrame* find_or_create_locked(FrameIdx frame_idx);
  void drop_connection_locked();
  void try_finish_inflight_locked();
  // 序列头门：排队帧两路到齐即裁决——预测+实测双 IDR 开门，否则整对丢弃（kHeadBarrier）
  void resolve_head_gate_locked();
  // 已丢帧登记（残包静默丢弃，不重复上报）
  void note_dropped_locked(FrameIdx frame_idx);
  bool is_dropped_locked(FrameIdx frame_idx) const;

  // 在途帧写出推进（连续部分写直到整帧发完/写不动/写错误——部分写不截断）。
  // track_progress 为假时不更新进展时戳（假死补发路径）。
  enum class WriteState { kDone, kBlocked, kError };
  WriteState write_out_locked(OutFrame& f, uint64_t now, bool track_progress);

  void emit_locked(UplinkEvent ev, FrameIdx frame_idx);

  UplinkSocket* sock_;
  std::function<uint64_t()> now_;
  UplinkSenderConfig cfg_;
  EventFn on_event_;

  mutable std::mutex mtx_;
  std::condition_variable work_cv_;
  bool work_pending_ = false;  // submit_* 置位、wait_for_work 消费
  bool connected_ = false;
  ConnEpoch conn_epoch_{};  // 每次建连 +1（kNewConnection 事件带出）
  bool stream_open_ = false;  // 序列头门：已发出合格双路 IDR 对
  int connect_failures_ = 0;
  bool has_inflight_ = false;
  OutFrame inflight_;
  bool has_queued_ = false;
  OutFrame queued_;
  uint64_t last_progress_ms_ = 0;

  // 已丢帧 frame_idx 小窗口（残包/迟到包静默丢弃，避免重复上报 kHeadBarrier）
  static constexpr int kDroppedWindow = 32;
  FrameIdx dropped_recent_[kDroppedWindow] = {};
  int dropped_n_ = 0;
  int dropped_pos_ = 0;
};

// ---- 链路状态记账（06 号）：对外报出连接状态与断开原因（Param "BigmodelLinkState"）----
// 状态串（07 号 UI 文案按此映射）：
//   connecting  连接中（起步/断连重连中）
//   connected   已连接（首次见到该 instance_id）
//   blip        网络闪断后已恢复（重连后 instance_id 相同）
//   restart     服务端重启后已恢复（重连后 instance_id 变了）
//   lost        连接丢失（连败升级 kLinkLost，仍在重试）
// 纯逻辑 + 内部互斥（reply 线程喂 on_hello、发送事件线程喂其余）。
class LinkStateTracker {
 public:
  void on_connecting();               // 断连/重连中
  void on_hello(uint64_t instance_id);  // HELLO 到达：按 instance_id 对比判 blip/restart
  void on_lost();                     // kLinkLost
  std::string value() const;

 private:
  mutable std::mutex mtx_;
  uint64_t last_instance_ = 0;
  bool have_instance_ = false;
  const char* state_ = "connecting";
};
