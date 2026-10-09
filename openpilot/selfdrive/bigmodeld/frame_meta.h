#pragma once

// 帧头元数据（16 号 (d)）：
//   t_eof             = road 帧 timestamp_eof（ns，调用方给）
//   warp_road/warp_wide = C++ 复刻 openpilot/common/transformations/model.py:65-70
//                       get_warp_matrix（输入 extrinsicsCalibration.rpyCalib + 相机内参常量），
//                       与 lean modeld 送进 frame_prepare 的 3×3 矩阵同义（行主序 f32，
//                       modeld.py:330,333 的 .astype(np.float32)）；
//                       宿主 golden 对照 Python ≤1e-5（gen_warp_golden.py → test_bigmodeld.cc）
//   traffic_convention = [1, 0]（lean modeld is_rhd=False 硬编码，modeld.py:321,336）
//   desire[8] = modeld DesireHelper 电平（set_model_inputs 进）→ fill 转 pulse 边沿
//     （MODEL_ABI §4.2：通道 0 恒 0、持续同 desire 只 pulse 一次）。
//   action_t[2] = modeld chestnut 公式直通（13 号 / research/02 §4，权威在 modeld）。
//
// 纯逻辑（仅 std），宿主单测直接编译。

#include <cstdint>
#include <mutex>

#include "frame_codec.h"

// C++ 复刻 get_warp_matrix（common/transformations/model.py:65-70）：
//   warp = K_cam @ view_frame_from_device_frame @ rot_from_euler(rpy) @ inv(K_model @ view_frame_from_device_frame)
//   view_frame_from_device_frame = device_frame_from_view_frame.T（camera.py:65-66）
//   bigmodel_frame=false → K_model = medmodel_intrinsics（road → MED）
//   bigmodel_frame=true  → K_model = sbigmodel_intrinsics（wide → SBIG）
// 输出 3×3 行主序 f32。rpy = [roll, pitch, yaw]（extrinsicsCalibration.rpyCalib）。
void get_warp_matrix(const float rpy[3], const float intrinsics[9], bool bigmodel_frame, float out[9]);

// 相机内参常量：openpilot/common/transformations/camera.py 的 _os_config
//（os04c10，1344×760：narrow_road fl=1522*3/4、wide_road=_os_fisheye fl=567/4*3；
// 与 16 号两路取流分辨率 1344×760 同源）
extern const float kNarrowRoadIntrinsics[9];
extern const float kWideRoadIntrinsics[9];

// 帧头元数据提供者：rpyCalib 由标定线程喂（extrinsicsCalibration），fill() 由取帧线程调。
class MetaProvider {
 public:
  MetaProvider();

  // extrinsicsCalibration.rpyCalib 更新时调用（任意线程）。
  void set_rpy(const float rpy[3]);

  // modeld 上行元数据（04 号 C-2，modelDataV2SP）更新时调用（任意线程）：
  // action_t = chestnut 公式（research/02 §4），desire_class = DH.desire 电平。
  void set_model_inputs(const float action_t[2], uint8_t desire_class);

  // 新序列首帧组帧前调用（双路 IDR 恢复/重连 = App 侧 new_seq，bgm1_server.cpp）：
  // MODEL_ABI §4.3 清 previousDesire，持续中的 desire 在新序列首帧重出 pulse。
  void reset_desire_latch();

  // 生成一帧的头元数据。t_eof = road 帧 timestamp_eof（ns）。
  // 只填 t_eof/desire/traffic_convention/action_t/warp_*；flags/frame_idx/road_len/wide_len 归调用方。
  void fill(uint64_t t_eof, bgm1::FrameHeader* out) const;

  // 当前 warp 矩阵（wide=false → road/MED，true → wide/SBIG），与 fill 写进帧头的同一份。
  // capture 线程据此在 C4 上 warp（协议 v2）。
  void warp(bool wide, float out[9]) const;

 private:
  void recompute_locked();

  mutable std::mutex mtx_;
  float rpy_[3] = {0.f, 0.f, 0.f};
  float warp_road_[9] = {0};
  float warp_wide_[9] = {0};
  float action_t_[2] = {0.f, 0.f};
  uint8_t desire_class_ = 0;
  mutable uint8_t prev_desire_class_ = 0;  // pulse 边沿 latch：每次 fill() 采样
};
