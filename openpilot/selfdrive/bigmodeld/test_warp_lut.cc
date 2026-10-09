// WarpLut 宿主单测（需 arm64/NEON，Apple Silicon 直接编）：查找表输出与 warpNv12 逐位一致，
// 矩阵/源几何变化必重建、源内容变化不需重建。
//   clang++ -std=c++17 -O1 test_warp_lut.cc warp_pack.cpp frame_meta.cpp frame_codec.cpp \
//           -o /tmp/test_warp_lut && /tmp/test_warp_lut
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "frame_meta.h"
#include "warp_pack.h"

using namespace chipmunk;

static int g_fails = 0;
#define CHECK(cond)                                         \
  do {                                                      \
    if (!(cond)) {                                          \
      g_fails++;                                            \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    }                                                       \
  } while (0)

struct Src {
  std::vector<uint8_t> buf;
  Nv12View v;
  Src(int stride) : buf((size_t)stride * kWarpSrcH * 3 / 2) {
    for (auto& b : buf) b = (uint8_t)rand();
    v.y = buf.data();
    v.uv = buf.data() + (size_t)stride * kWarpSrcH;
    v.stride_y = v.stride_uv = stride;
  }
};

struct Out {
  static constexpr int kStride = 576;  // 带填充行跨，验证只写有效区
  std::vector<uint8_t> y = std::vector<uint8_t>(kStride * kModelH, 0xEE);
  std::vector<uint8_t> uv = std::vector<uint8_t>(kStride * kModelH / 2, 0xEE);
  Nv12Out o() { return Nv12Out{y.data(), uv.data(), kStride, kStride}; }
  bool operator==(const Out& b) const { return y == b.y && uv == b.uv; }
};

static bool lut_matches(WarpLut& lut, const Nv12View& src, const float m[9]) {
  Out ref, got;
  CHECK(warpNv12(src, m, ref.o()));
  CHECK(lut.warp(src, m, got.o()));
  return ref == got;
}

int main() {
  float road[9], wide[9], road2[9];
  const float rpy[3] = {0.f, 0.03f, 0.01f}, rpy2[3] = {0.001f, 0.031f, 0.0105f};
  get_warp_matrix(rpy, kNarrowRoadIntrinsics, false, road);
  get_warp_matrix(rpy, kWideRoadIntrinsics, true, wide);
  get_warp_matrix(rpy2, kNarrowRoadIntrinsics, false, road2);

  Src a(kWarpSrcW), b(kWarpSrcW), padded(1408);
  WarpLut lut;
  CHECK(lut_matches(lut, a.v, road));     // 首次建表
  CHECK(lut_matches(lut, b.v, road));     // 换源内容、同矩阵：复用表
  CHECK(lut_matches(lut, a.v, wide));     // 换矩阵：重建
  CHECK(lut_matches(lut, a.v, road2));    // 标定微调：重建
  CHECK(lut_matches(lut, padded.v, road2));  // 换源行跨：重建

  Nv12View bad = a.v;
  bad.y = nullptr;
  Out o;
  CHECK(!lut.warp(bad, road, o.o()));
  CHECK(lut_matches(lut, a.v, road));     // 非法输入后仍可用

  printf("warp_lut: %s\n", g_fails ? "FAIL" : "ALL PASS");
  return g_fails ? 1 : 0;
}
