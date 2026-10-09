#pragma once

// C4 本机分段计时：以 road 的 timestamp_eof 为零点，沿关键路径逐段首尾相接
//   取帧(eof→VisionIPC 收到) → warp → 配对等待(等另一路) → 硬编(提交→两路都出包)
//   → 发送(出包→FRAME 写完) → 等 REPLY(写完→REPLY 收到)
// 六段之和 = REPLY 收到 − eof。时钟统一 nanos_since_boot（与 timestamp_eof 同源）。
// 按 frame_idx 落环形槽（叶子锁）；槽被新帧顶掉后，旧帧的迟到事件/REPLY 一律忽略。
// 纯逻辑（仅 std），宿主单测 test_bigmodeld.cc。

#include <cstdint>
#include <mutex>

#include "frame_ids.h"

struct StageMs {
  float capture = 0, warp = 0, pair_wait = 0, encode = 0, send = 0, reply_wait = 0;
};

class FrameStageLog {
 public:
  static constexpr uint32_t kSlots = 64;  // ≈3.2 s @20 Hz，远大于在途帧数

  void on_submit(FrameIdx idx, uint64_t eof_ns, uint64_t recv_ns, uint64_t warped_ns, uint64_t submit_ns) {
    std::lock_guard<std::mutex> lk(mtx_);
    slot(idx) = Entry{idx, eof_ns, recv_ns, warped_ns, submit_ns, 0, 0, true};
  }

  void on_encoded(FrameIdx idx, uint64_t ns) {
    std::lock_guard<std::mutex> lk(mtx_);
    Entry* e = find(idx);
    if (e && ns > e->encoded_ns) e->encoded_ns = ns;
  }

  void on_sent(FrameIdx idx, uint64_t ns) {
    std::lock_guard<std::mutex> lk(mtx_);
    Entry* e = find(idx);
    if (e) e->sent_ns = ns;
  }

  // 各段齐全才返回 true（每帧只报一次）
  bool on_reply(FrameIdx idx, uint64_t ns, StageMs* out) {
    std::lock_guard<std::mutex> lk(mtx_);
    Entry* e = find(idx);
    if (!e || e->encoded_ns == 0 || e->sent_ns == 0) return false;
    auto ms = [](uint64_t a, uint64_t b) { return b > a ? float(double(b - a) / 1e6) : 0.f; };
    *out = StageMs{ms(e->eof_ns, e->recv_ns),     ms(e->recv_ns, e->warped_ns), ms(e->warped_ns, e->submit_ns),
                   ms(e->submit_ns, e->encoded_ns), ms(e->encoded_ns, e->sent_ns), ms(e->sent_ns, ns)};
    e->live = false;
    return true;
  }

 private:
  struct Entry {
    FrameIdx idx{};
    uint64_t eof_ns = 0, recv_ns = 0, warped_ns = 0, submit_ns = 0, encoded_ns = 0, sent_ns = 0;
    bool live = false;
  };
  Entry& slot(FrameIdx idx) { return ring_[u32(idx) % kSlots]; }
  Entry* find(FrameIdx idx) {
    Entry& e = slot(idx);
    return e.live && e.idx == idx ? &e : nullptr;
  }

  std::mutex mtx_;
  Entry ring_[kSlots];
};
