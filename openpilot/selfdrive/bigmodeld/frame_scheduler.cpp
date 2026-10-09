#include "frame_scheduler.h"

void FrameIndexer::reset() {
  have_base_ = false;
  sof_base_ns_ = 0;
  last_idx_ = 0;
}

FrameIdx FrameIndexer::index(uint64_t road_sof_ns) {
  if (!have_base_) {
    sof_base_ns_ = road_sof_ns;
    have_base_ = true;
    last_idx_ = 0;
    return FrameIdx{0};
  }

  // 与 (delta + 25 ms) / 50 ms 等价，避免 delta + 25 ms 的整数溢出。
  const uint64_t delta = road_sof_ns >= sof_base_ns_ ? road_sof_ns - sof_base_ns_ : 0;
  uint64_t next_idx = delta / kFramePeriodNs;
  if (delta % kFramePeriodNs >= kFramePeriodNs / 2) next_idx++;

  // SOF 正常间隔为 50~66 ms；仍防止异常近邻/重复时间戳落在同一槽。
  if (next_idx <= last_idx_) next_idx = last_idx_ + 1;
  last_idx_ = next_idx;
  return FrameIdx{static_cast<uint32_t>(last_idx_)};
}

SchedStep FrameScheduler::start_new_sequence(bool drop_detected) {
  SchedStep s;
  s.drop_detected = drop_detected;
  s.request_keyframe_road = true;
  s.request_keyframe_wide = true;
  pending_req_road_ = true;
  pending_req_wide_ = true;
  seq_pos_ = 0;
  next_idr_seq_road_ = 0;
  next_idr_seq_wide_ = 0;
  return s;
}

SchedStep FrameScheduler::on_connect() {
  SchedStep s = start_new_sequence(false);
  // frame_idx 重新编号，去重窗口作废
  recent_drop_n_ = 0;
  recent_drop_pos_ = 0;
  return s;
}

SchedStep FrameScheduler::on_frame_submit(FrameIdx frame_idx) {
  (void)frame_idx;  // frame_idx 是时间槽编号；GOP 相位按实际提交帧计数，不按空槽数计
  SchedStep s;

  // 新序列第 10 帧给 wide 再补一次 I 帧（恢复「wide 比 road 晚 10 帧」相位）
  if (seq_pos_ == 10) {
    s.request_keyframe_wide = true;
    pending_req_wide_ = true;
  }

  // 本帧 IDR 预测 = request 落点（request 先于本帧 encode_frame，21 号「下一帧立即 I 帧」）
  // ∪ GOP 20 基准（上一个 IDR 后第 20 帧）。预测即 FRAME 头 flags 语义。
  s.road_idr = pending_req_road_ || (seq_pos_ == next_idr_seq_road_);
  if (s.road_idr) {
    pending_req_road_ = false;
    next_idr_seq_road_ = seq_pos_ + 20;
  }
  s.wide_idr = pending_req_wide_ || (seq_pos_ == next_idr_seq_wide_);
  if (s.wide_idr) {
    pending_req_wide_ = false;
    next_idr_seq_wide_ = seq_pos_ + 20;
  }

  seq_pos_++;
  return s;
}

// 同一 frame 的重复显式上报只报一次：查窗口并在窗口内登记（新报才登记）。
bool FrameScheduler::drop_already_reported(FrameIdx frame_idx) {
  for (int i = 0; i < recent_drop_n_; i++) {
    if (recent_drop_[i] == frame_idx) return true;
  }
  recent_drop_[recent_drop_pos_] = frame_idx;
  recent_drop_pos_ = (recent_drop_pos_ + 1) % kDropDedupWindow;
  if (recent_drop_n_ < kDropDedupWindow) recent_drop_n_++;
  return false;
}

SchedStep FrameScheduler::on_frame_dropped(FrameIdx frame_idx) {
  // 码流断档统一入口（kDrop / kHeadBarrier）：
  // 同一时间槽重复显式上报只处理一次，避免重复开序列/发 request。
  if (drop_already_reported(frame_idx)) return SchedStep{};

  return start_new_sequence(true);
}

SchedStep FrameScheduler::on_stream_gap() {
  // 编码输出被吞（MetaCache kGapMiss 情形 C）：无 frame_idx 键、无重复上报问题
  // （同 frame_id 不会二次输出），不参与去重，每次照常开新序列。
  return start_new_sequence(true);
}

