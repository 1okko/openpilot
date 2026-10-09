#pragma once

// 强类型 ID（19 号 #1）：三个 ID 都是 uint32_t 但键空间不同，裸类型混用出过事故
// （16 号根因之一 = frame_id / frame_idx 混用）。enum class 零开销，编译期挡住互串。
// 线协议结构体（frame_codec.h）仍用裸 uint32_t，仅在边界处经 u32()/构造转换。

#include <cstdint>

enum class FrameIdx : uint32_t {};    // 连接内 50 ms 时间槽编号（进 FRAME 头）
enum class CamFrameId : uint32_t {};  // 各路相机自己的 frame_id（两路计数器独立漂移）
enum class ConnEpoch : uint32_t {};   // 连接代号（sender 建连次数）

template <typename IdT>
constexpr uint32_t u32(IdT v) { return static_cast<uint32_t>(v); }
