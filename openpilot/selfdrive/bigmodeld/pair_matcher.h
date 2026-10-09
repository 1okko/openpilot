#pragma once

// 跨相机配对状态机（16 号诊断修复 2026-09-28）：按 timestamp_sof 邻近配对，
// **不按 frame_id 相等**。真机实测依据（C4 probe_fid/probe_mono，2026-09-28）：
//   - 两路 frame_id 是各自独立的出帧计数器（每出帧 +1、严格单调，但出帧率不同：
//     road ≈ 18.8 Hz、wide ≈ 18.3 Hz），差值以 ~0.5 帧/s 持续漂移、永不相等；
//     request_id 同样是各路自己的序列（与 frame_id 恒差 ife_buf_depth）。
//   - 同帧 SOF 对齐（|Δsof| ≤ 1.3 ms），帧间 ≥ 49 ms。
//   - stock modeld 同样按 timestamp_sof 配对、从不比 frame_id
//     （modeld.py:288-312，10 ms 为 out-of-sync 判据）。
// 故配对键 = |sof_a − sof_b| ≤ kPairToleranceNs。
//
// 单槽语义（每路一槽）：
//   - 新帧到来：先与对路槽配对（成对即两帧出槽）；不成对则置换我路旧帧
//     （旧帧死亡：其同窗对路帧要么早已成对、要么从未到来且新帧已推进）。
//   - 对路槽内旧帧若已越过新帧的配对窗口（future 我路帧只会更晚），立即驱逐。
//   - 不允许错配：帧距 ≥ 49 ms > 2×容差，最近邻即真同帧对。
//   - 已知损耗：同路置换先于对路帧到达（对路迟到 > ~50 ms）时丢一对，属
//     单槽取舍；两路同窗帧共存未配对即证明非同帧。
//
// 调用方约定：两侧死亡都只做计数/日志并归还缓冲，不上报 FrameScheduler、不触发
// 新序列或 request_keyframe。只有 road 死亡帧可用其 timestamp_sof 建立/推进 frame_idx
// 时间槽；wide 的 SOF 不参与编号。时间槽空洞是相机缺帧/未配对的常态，不表示码流断档。

#include <cstdint>

namespace bgm {

constexpr uint64_t kPairToleranceNs = 10 * 1000 * 1000ULL;  // 10 ms（modeld 同口径）

inline bool pairable(uint64_t sof_a, uint64_t sof_b) {
  uint64_t d = sof_a > sof_b ? sof_a - sof_b : sof_b - sof_a;
  return d <= kPairToleranceNs;
}

// T = 调用方帧负载（main.cc 里是编码缓冲 + VisionIpcBufExtra）。
template <typename T>
class PairMatcher {
 public:
  struct Frame {
    uint32_t frame_id = 0;
    uint64_t timestamp_sof = 0;
    T payload = T();
  };

  struct Actions {
    bool kill_road = false;   // road 帧死亡（调用方只计数/日志 + 归还缓冲）
    Frame road_dead;
    bool kill_wide = false;   // wide 帧死亡（调用方只计数/日志 + 归还缓冲）
    Frame wide_dead;
    bool pair = false;        // 成对（两帧出槽，进编码器）
    Frame road, wide;
  };

  // 每路新帧到达时调用一次；is_road = true 表示 road（窄前视主路）。
  Actions push(bool is_road, Frame f) {
    Actions a;
    Slot& mine = slots_[is_road ? 0 : 1];
    Slot& other = slots_[is_road ? 1 : 0];

    if (other.has && pairable(f.timestamp_sof, other.f.timestamp_sof)) {
      if (mine.has) kill(a, is_road, mine.f);  // 置换死（同窗对路帧已与新帧成对）
      a.pair = true;
      if (is_road) {
        a.road = f;
        a.wide = other.f;
      } else {
        a.road = other.f;
        a.wide = f;
      }
      mine.has = false;
      other.has = false;
      return a;
    }

    if (mine.has) kill(a, is_road, mine.f);  // 置换死
    mine.f = f;
    mine.has = true;

    // 对路旧帧窗口已过（我路后续帧只会更晚）：驱逐
    if (other.has && other.f.timestamp_sof + kPairToleranceNs < f.timestamp_sof) {
      kill(a, !is_road, other.f);
      other.has = false;
    }
    return a;
  }

  // 测试/统计用：槽内是否有未决帧
  bool has_pending(bool is_road) const { return slots_[is_road ? 0 : 1].has; }

 private:
  struct Slot {
    bool has = false;
    Frame f;
  };

  static void kill(Actions& a, bool is_road, const Frame& f) {
    if (is_road) {
      a.kill_road = true;
      a.road_dead = f;
    } else {
      a.kill_wide = true;
      a.wide_dead = f;
    }
  }

  Slot slots_[2];
};

}  // namespace bgm
