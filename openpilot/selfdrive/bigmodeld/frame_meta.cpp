#include "frame_meta.h"

#include <cmath>
#include <cstring>

namespace {

// 3×3 行主序 double 小工具（仅本文件用）
void mat_mul(const double* a, const double* b, double* out) {
  double t[9];
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 3; c++) {
      t[3 * r + c] = a[3 * r + 0] * b[0 + c] + a[3 * r + 1] * b[3 + c] + a[3 * r + 2] * b[6 + c];
    }
  }
  std::memcpy(out, t, sizeof t);
}

bool mat_inv(const double* m, double* out) {
  const double a = m[0], b = m[1], c = m[2];
  const double d = m[3], e = m[4], f = m[5];
  const double g = m[6], h = m[7], i = m[8];
  const double A = e * i - f * h, B = -(d * i - f * g), C = d * h - e * g;
  const double det = a * A + b * B + c * C;
  if (det == 0.0) return false;
  const double inv = 1.0 / det;
  out[0] = A * inv;
  out[1] = -(b * i - c * h) * inv;
  out[2] = (b * f - c * e) * inv;
  out[3] = B * inv;
  out[4] = (a * i - c * g) * inv;
  out[5] = -(a * f - c * d) * inv;
  out[6] = C * inv;
  out[7] = -(a * h - b * g) * inv;
  out[8] = (a * e - b * d) * inv;
  return true;
}

// transformations.py:141 euler2rot_single：Rz(yaw) @ Ry(pitch) @ Rx(roll)（Z-Y-X）
void rot_from_euler(const double euler[3], double* out) {
  const double phi = euler[0], theta = euler[1], psi = euler[2];  // roll, pitch, yaw
  const double cx = std::cos(phi), sx = std::sin(phi);
  const double cy = std::cos(theta), sy = std::sin(theta);
  const double cz = std::cos(psi), sz = std::sin(psi);
  const double rx[9] = {1, 0, 0, 0, cx, -sx, 0, sx, cx};
  const double ry[9] = {cy, 0, sy, 0, 1, 0, -sy, 0, cy};
  const double rz[9] = {cz, -sz, 0, sz, cz, 0, 0, 0, 1};
  double t[9];
  mat_mul(rz, ry, t);
  mat_mul(t, rx, out);
}

// camera.py:65-66 view_frame_from_device_frame = device_frame_from_view_frame.T
//（device x→forward,y→right,z→down；view x→right,y→down,z→forward）
const double kViewFromDevice[9] = {0, 1, 0,
                                   0, 0, 1,
                                   1, 0, 0};

// model.py:14-18 medmodel_intrinsics（MEDMODEL_CY = 47.6）
const double kMedmodelIntrinsics[9] = {910, 0, 256,
                                       0, 910, 47.6,
                                       0, 0, 1};

// model.py:36-40 sbigmodel_intrinsics（cy = 0.5 * (256 + 47.6) = 151.8）
const double kSbigmodelIntrinsics[9] = {455, 0, 256,
                                        0, 455, 151.8,
                                        0, 0, 1};

}  // namespace

const float kNarrowRoadIntrinsics[9] = {1141.5f, 0.f, 672.f,
                                        0.f, 1141.5f, 380.f,
                                        0.f, 0.f, 1.f};
const float kWideRoadIntrinsics[9] = {425.25f, 0.f, 672.f,
                                      0.f, 425.25f, 380.f,
                                      0.f, 0.f, 1.f};

void get_warp_matrix(const float rpy[3], const float intrinsics[9], bool bigmodel_frame, float out[9]) {
  const double euler[3] = {rpy[0], rpy[1], rpy[2]};
  double rot[9];
  rot_from_euler(euler, rot);

  double k_cam[9], k_model[9];
  for (int i = 0; i < 9; i++) {
    k_cam[i] = intrinsics[i];
    k_model[i] = bigmodel_frame ? kSbigmodelIntrinsics[i] : kMedmodelIntrinsics[i];
  }

  double tmp[9], camera_from_calib[9], calib_from_model[9], warp[9];
  mat_mul(k_cam, kViewFromDevice, tmp);
  mat_mul(tmp, rot, camera_from_calib);            // camera_from_calib = K_cam @ V @ R
  mat_mul(k_model, kViewFromDevice, tmp);
  if (!mat_inv(tmp, calib_from_model)) {           // calib_from_model = inv(K_model @ V)
    for (int i = 0; i < 9; i++) out[i] = 0.f;
    return;
  }
  mat_mul(camera_from_calib, calib_from_model, warp);

  for (int i = 0; i < 9; i++) out[i] = (float)warp[i];
}

MetaProvider::MetaProvider() {
  recompute_locked();
}

void MetaProvider::warp(bool wide, float out[9]) const {
  std::lock_guard<std::mutex> lk(mtx_);
  std::memcpy(out, wide ? warp_wide_ : warp_road_, sizeof warp_road_);
}

void MetaProvider::recompute_locked() {
  get_warp_matrix(rpy_, kNarrowRoadIntrinsics, false, warp_road_);
  get_warp_matrix(rpy_, kWideRoadIntrinsics, true, warp_wide_);
}

void MetaProvider::set_rpy(const float rpy[3]) {
  std::lock_guard<std::mutex> lk(mtx_);
  rpy_[0] = rpy[0];
  rpy_[1] = rpy[1];
  rpy_[2] = rpy[2];
  recompute_locked();
}

void MetaProvider::set_model_inputs(const float action_t[2], uint8_t desire_class) {
  std::lock_guard<std::mutex> lk(mtx_);
  action_t_[0] = action_t[0];
  action_t_[1] = action_t[1];
  desire_class_ = desire_class;
}

void MetaProvider::reset_desire_latch() {
  std::lock_guard<std::mutex> lk(mtx_);
  prev_desire_class_ = 0;
}

void MetaProvider::fill(uint64_t t_eof, bgm1::FrameHeader* out) const {
  std::lock_guard<std::mutex> lk(mtx_);
  out->t_eof = t_eof;
  // desire 电平 → pulse 边沿（MODEL_ABI §4.2）：d!=0 且换值才 one-hot 一次，通道 0 恒 0。
  // latch 每 fill 采样一次（fill=一帧模型执行输入，对齐上游 model.run() 内 prev_desire 语义）。
  for (int i = 0; i < 8; i++) out->desire[i] = 0.f;
  const uint8_t d = desire_class_;
  if (d != 0 && d != prev_desire_class_ && d < 8) out->desire[d] = 1.f;
  prev_desire_class_ = d;
  out->traffic_convention[0] = 1.f;                           // lean modeld is_rhd=False → [1, 0]
  out->traffic_convention[1] = 0.f;
  out->action_t[0] = action_t_[0];
  out->action_t[1] = action_t_[1];
  std::memcpy(out->warp_road, warp_road_, sizeof warp_road_);
  std::memcpy(out->warp_wide, warp_wide_, sizeof warp_wide_);
}
