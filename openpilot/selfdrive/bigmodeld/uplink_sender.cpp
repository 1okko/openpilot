#include "uplink_sender.h"

#include <cstring>

namespace {

inline void put_u32_le(uint8_t* p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

}  // namespace

UplinkSender::UplinkSender(UplinkSocket* sock, std::function<uint64_t()> now_ms,
                           UplinkSenderConfig cfg, EventFn on_event)
    : sock_(sock), now_(std::move(now_ms)), cfg_(cfg), on_event_(std::move(on_event)) {}

void UplinkSender::emit_locked(UplinkEvent ev, FrameIdx frame_idx) {
  if (on_event_) on_event_(UplinkEventInfo{ev, frame_idx, conn_epoch_});
}

UplinkSender::OutFrame* UplinkSender::find_or_create_locked(FrameIdx frame_idx) {
  if (has_inflight_ && inflight_.frame_idx == frame_idx) return &inflight_;
  if (has_queued_ && queued_.frame_idx == frame_idx) return &queued_;
  // 迟到的旧包（该帧已被覆盖/屏障丢弃并上报过，或残包）：静默丢弃
  if (is_dropped_locked(frame_idx)) return nullptr;
  if (has_inflight_ && frame_idx < inflight_.frame_idx) return nullptr;
  if (has_queued_ && frame_idx < queued_.frame_idx) return nullptr;
  // 新包覆盖排队槽 = 丢弃旧包（丢帧）；序列头门关门期间丢的是待裁决帧（非断档）
  if (has_queued_) {
    const FrameIdx dead_idx = queued_.frame_idx;
    const bool gap = stream_open_;
    note_dropped_locked(dead_idx);
    emit_locked(gap ? UplinkEvent::kDrop : UplinkEvent::kHeadBarrier, dead_idx);
    if (gap) stream_open_ = false;  // 断档后首帧必须重新双路 IDR 开流（18 号 #1）
    has_queued_ = false;
  }
  queued_ = OutFrame{};
  queued_.frame_idx = frame_idx;
  has_queued_ = true;
  return &queued_;
}

void UplinkSender::note_dropped_locked(FrameIdx frame_idx) {
  dropped_recent_[dropped_pos_] = frame_idx;
  dropped_pos_ = (dropped_pos_ + 1) % kDroppedWindow;
  if (dropped_n_ < kDroppedWindow) dropped_n_++;
}

bool UplinkSender::is_dropped_locked(FrameIdx frame_idx) const {
  for (int i = 0; i < dropped_n_; i++) {
    if (dropped_recent_[i] == frame_idx) return true;
  }
  return false;
}

void UplinkSender::resolve_head_gate_locked() {
  if (stream_open_ || !has_queued_) return;
  if (!queued_.has_road || !queued_.has_wide) return;  // 整对到齐才裁决（缺半 = 等/被覆盖）
  if (queued_.road_idr_actual && queued_.wide_idr_actual &&
      queued_.road_idr_predicted && queued_.wide_idr_predicted) {
    stream_open_ = true;  // 合格双路 IDR 对开流（断档后/连接首帧）
    return;
  }
  // 不合格对整对丢弃（非断档：什么都没写出去），上报补双路 request，直到合格对上线
  const FrameIdx idx = queued_.frame_idx;
  note_dropped_locked(idx);
  has_queued_ = false;
  queued_ = OutFrame{};
  emit_locked(UplinkEvent::kHeadBarrier, idx);
}

void UplinkSender::submit_road(ConnEpoch conn_epoch, FrameIdx frame_idx, const bgm1::FrameHeader& hdr,
                               const uint8_t* road, size_t road_len,
                               bool road_idr_actual, bool road_idr_predicted,
                               bool wide_idr_predicted) {
  std::lock_guard<std::mutex> lk(mtx_);
  // 断连期间的帧静默丢弃（重连即新序列，frame_idx 归零）；旧连接代号的迟到提交
  // 同样静默丢弃（18 号 #2：不得把旧 frame_idx 混进新连接）
  if (!connected_ || conn_epoch != conn_epoch_) {
    return;
  }

  OutFrame* f = find_or_create_locked(frame_idx);
  if (f == nullptr || f->has_road) return;

  bgm1::FrameHeader h = hdr;
  h.frame_idx = u32(frame_idx);
  h.road_len = (uint32_t)road_len;
  h.wide_len = 0;  // 线上 wide_len 在 chunk2（10 号布局：头里 len 只表示 road 段）
  h.flags = (uint16_t)((road_idr_actual ? bgm1::kFlagRoadIdr : 0) |
                       (wide_idr_predicted ? bgm1::kFlagWideIdr : 0));

  f->chunk1.resize(bgm1::kFrameHdrSize + road_len);
  bgm1::pack_frame_header(h, f->chunk1.data());
  std::memcpy(f->chunk1.data() + bgm1::kFrameHdrSize, road, road_len);
  f->has_road = true;
  f->road_idr_actual = road_idr_actual;
  f->road_idr_predicted = road_idr_predicted;
  f->wide_idr_predicted = wide_idr_predicted;
  resolve_head_gate_locked();
  work_pending_ = true;
  work_cv_.notify_one();
}

void UplinkSender::submit_wide(ConnEpoch conn_epoch, FrameIdx frame_idx, const uint8_t* wide,
                               size_t wide_len, bool wide_actual_idr) {
  std::lock_guard<std::mutex> lk(mtx_);
  if (!connected_ || conn_epoch != conn_epoch_) {
    return;
  }

  OutFrame* f = find_or_create_locked(frame_idx);
  if (f == nullptr || f->has_wide) return;
  f->chunk2.resize(sizeof(uint32_t) + wide_len + bgm1::kMacSize);
  put_u32_le(f->chunk2.data(), (uint32_t)wide_len);
  std::memcpy(f->chunk2.data() + sizeof(uint32_t), wide, wide_len);
  std::memset(f->chunk2.data() + sizeof(uint32_t) + wide_len, 0, bgm1::kMacSize);
  f->has_wide = true;
  f->wide_idr_actual = wide_actual_idr;
  resolve_head_gate_locked();
  work_pending_ = true;
  work_cv_.notify_one();
}

bool UplinkSender::wait_for_work(int timeout_ms) {
  std::unique_lock<std::mutex> lk(mtx_);
  const bool woke = work_cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms), [this] { return work_pending_; });
  work_pending_ = false;
  return woke;
}

void UplinkSender::drop_connection_locked() {
  sock_->close();
  connected_ = false;
  stream_open_ = false;  // 重连后序列头门重新关门（连接首帧必须双路 IDR 对）
  // 旧帧不得带进新连接（重连即新序列、frame_idx 归零）：静默清空
  has_inflight_ = false;
  has_queued_ = false;
  inflight_ = OutFrame{};
  queued_ = OutFrame{};
}

void UplinkSender::try_finish_inflight_locked() {
  if (!has_inflight_) return;
  // 写得动就发完（含连续部分写，不撕裂），写不动/写错才截断（drop_connection_locked 清残帧）
  if (write_out_locked(inflight_, 0, false) == WriteState::kDone) {
    FrameIdx idx = inflight_.frame_idx;
    has_inflight_ = false;
    emit_locked(UplinkEvent::kFrameSent, idx);
  }
}

UplinkSender::WriteState UplinkSender::write_out_locked(OutFrame& f, uint64_t now, bool track_progress) {
  while (true) {
    if (!f.chunk1_done) {
      int n = sock_->write_some(f.chunk1.data() + f.sent1, f.chunk1.size() - f.sent1);
      if (n < 0) return WriteState::kError;
      if (n == 0) return WriteState::kBlocked;
      f.sent1 += (size_t)n;
      if (track_progress) last_progress_ms_ = now;
      if (f.sent1 < f.chunk1.size()) continue;  // 部分写继续推进
      f.chunk1_done = true;
    }
    if (!f.has_wide) return WriteState::kBlocked;  // 等 wide 包（road 出包即发：chunk1 已在路上）
    int n = sock_->write_some(f.chunk2.data() + f.sent2, f.chunk2.size() - f.sent2);
    if (n < 0) return WriteState::kError;
    if (n == 0) return WriteState::kBlocked;
    f.sent2 += (size_t)n;
    if (track_progress) last_progress_ms_ = now;
    if (f.sent2 < f.chunk2.size()) continue;
    return WriteState::kDone;
  }
}

bool UplinkSender::step() {
  uint64_t now = now_();

  {
    std::lock_guard<std::mutex> lk(mtx_);
    if (connected_) {
      bool progress = false;

      // 序列头门：关门期间排队帧两路到齐即裁决（不合格整对丢弃、合格开流）
      if (!stream_open_) resolve_head_gate_locked();

      // 排队槽转在途（只有门已开且 road 段到齐才起发——road 出包即发）
      if (stream_open_ && !has_inflight_ && has_queued_ && queued_.has_road) {
        inflight_ = std::move(queued_);
        has_queued_ = false;
        has_inflight_ = true;
        last_progress_ms_ = now;  // 新在途帧起点，空闲期不计入假死
        progress = true;
      }
      if (!has_inflight_) return progress;

      const FrameIdx idx = inflight_.frame_idx;
      const size_t before = inflight_.sent1 + inflight_.sent2;
      WriteState ws = write_out_locked(inflight_, now, true);
      if (ws == WriteState::kDone) {
        has_inflight_ = false;
        emit_locked(UplinkEvent::kFrameSent, idx);
        return true;
      }
      if (ws == WriteState::kError) {
        drop_connection_locked();
        return progress;
      }
      progress |= (inflight_.sent1 + inflight_.sent2) != before;

      // 假死 = 对在途帧 ≥200 ms 无进展（写阻塞或 wide 缺失）：
      // 发完在途帧（写得动就发完）或截断，随后重连，上报重置
      if (now - last_progress_ms_ >= cfg_.stall_ms) {
        emit_locked(UplinkEvent::kStall, idx);
        try_finish_inflight_locked();
        drop_connection_locked();
      }
      return progress;
    }
  }

  // 未连接：建连在锁外做（最长 connect_timeout_ms），submit_* 走「未连接即丢」快路径
  bool ok = sock_->connect(cfg_.connect_timeout_ms);
  std::lock_guard<std::mutex> lk(mtx_);
  if (ok) {
    connected_ = true;
    stream_open_ = false;  // 连接首帧必须双路 IDR 对（序列头门）
    conn_epoch_ = ConnEpoch{u32(conn_epoch_) + 1};  // 新连接代号：旧连接的迟到提交从此被拒
    // 已丢帧 tombstone 也是旧连接账（frame_idx 归零重编号）：不清会静默吃掉
    // 新连接里的同 id 帧（stall3 实测：旧 id 9/10/15/16/169/170 误杀新连接帧，
    // GOP 相位偏移 + 解码断链，且全程无上报）
    dropped_n_ = 0;
    dropped_pos_ = 0;
    connect_failures_ = 0;
    last_progress_ms_ = now_();
    emit_locked(UplinkEvent::kNewConnection, FrameIdx{});
    return true;
  }
  if (++connect_failures_ == cfg_.max_connect_failures) {
    emit_locked(UplinkEvent::kLinkLost, FrameIdx{});
  }
  return false;
}

void UplinkSender::notify_disconnect() {
  std::lock_guard<std::mutex> lk(mtx_);
  if (connected_) drop_connection_locked();
}

void UplinkSender::notify_stream_gap() {
  // 编码输出缺帧/段长超限 = 码流断档：断档后首帧必须重新双路 IDR 开流。
  // 在途帧属于断档之前（写得动就发完），关门只约束之后的排队帧。
  std::lock_guard<std::mutex> lk(mtx_);
  stream_open_ = false;
}

bool UplinkSender::connected() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return connected_;
}

ConnEpoch UplinkSender::conn_epoch() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return conn_epoch_;
}

// ---- LinkStateTracker（06 号）----

void LinkStateTracker::on_connecting() {
  std::lock_guard<std::mutex> lk(mtx_);
  state_ = "connecting";
}

void LinkStateTracker::on_hello(uint64_t instance_id) {
  std::lock_guard<std::mutex> lk(mtx_);
  if (have_instance_) state_ = (instance_id == last_instance_) ? "blip" : "restart";
  else state_ = "connected";
  last_instance_ = instance_id;
  have_instance_ = true;
}

void LinkStateTracker::on_lost() {
  std::lock_guard<std::mutex> lk(mtx_);
  state_ = "lost";
}

std::string LinkStateTracker::value() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return state_;
}
