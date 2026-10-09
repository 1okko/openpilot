// C4 warpNv12 + 手机 pack 的端到端对照：warpNv12 → 解交织成 packed6 == tinygrad make_frame_prepare golden
// （fixtures/warp_ref_m*.bin，make_warp_golden.py 生成）。m3（平局压力矩阵）逐位一致，m1/m2 允许零星 tie 翻转。
//   clang++ -std=c++17 -O1 test_warp_golden.cc warp_pack.cpp -o /tmp/test_warp_golden && /tmp/test_warp_golden
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "warp_pack.h"

using namespace chipmunk;

constexpr int kW = kWarpSrcW, kH = kWarpSrcH;
constexpr long kMaxTieFlips = 32;  // 0.016% of 196608

const float kM1[9] = {2.6f, 0.05f, 120.f, 0.02f, 2.6f, -20.f, 3e-5f, -1e-5f, 1.f};
const float kM2[9] = {-2.5f, 0.1f, 900.f, 0.03f, -2.4f, 600.f, 2e-5f, 3e-5f, 1.f};
const float kM3[9] = {2.5f, 0.0f, 0.5f, 0.0f, 2.5f, 0.5f, 0.0f, 0.0f, 1.f};

// 与 packNv12（手机，chipmunk）同序：Y00, Y10, Y01, Y11, U, V
static void pack6(const uint8_t* y, const uint8_t* uv, uint8_t* out) {
  const size_t plane = kPackW * kPackH;
  for (int i = 0; i < kPackH; i++)
    for (int j = 0; j < kPackW; j++) {
      const size_t o = (size_t)i * kPackW + j;
      out[0 * plane + o] = y[(size_t)(2 * i) * kModelW + 2 * j];
      out[1 * plane + o] = y[(size_t)(2 * i + 1) * kModelW + 2 * j];
      out[2 * plane + o] = y[(size_t)(2 * i) * kModelW + 2 * j + 1];
      out[3 * plane + o] = y[(size_t)(2 * i + 1) * kModelW + 2 * j + 1];
      out[4 * plane + o] = uv[(size_t)i * kModelW + 2 * j];
      out[5 * plane + o] = uv[(size_t)i * kModelW + 2 * j + 1];
    }
}

int main(int argc, char** argv) {
  const std::string dir = argc > 1 ? argv[1] : "fixtures";
  std::vector<uint8_t> y((size_t)kW * kH), uv((size_t)kW * kH / 2);
  uint32_t s = 0x9E3779B9u;  // 与 make_warp_golden.py 的 lcg_frame 同式
  for (auto& b : y) { s = s * 1664525u + 1013904223u; b = (uint8_t)(s >> 24); }
  for (auto& b : uv) { s = s * 1664525u + 1013904223u; b = (uint8_t)(s >> 24); }
  Nv12View src;
  src.y = y.data();
  src.uv = uv.data();

  int fails = 0;
  const float* mats[3] = {kM1, kM2, kM3};
  const char* names[3] = {"m1", "m2", "m3"};
  for (int mi = 0; mi < 3; mi++) {
    std::vector<uint8_t> ny((size_t)kModelW * kModelH), nuv((size_t)kModelW * kModelH / 2), got(kPacked6Bytes),
        gold(kPacked6Bytes);
    if (!warpNv12(src, mats[mi], Nv12Out{ny.data(), nuv.data()})) { printf("FAIL warpNv12 %s\n", names[mi]); fails++; continue; }
    pack6(ny.data(), nuv.data(), got.data());
    FILE* f = fopen((dir + "/warp_ref_" + names[mi] + ".bin").c_str(), "rb");
    if (!f || fread(gold.data(), 1, kPacked6Bytes, f) != kPacked6Bytes) { printf("FAIL 读不到 golden %s\n", names[mi]); fails++; if (f) fclose(f); continue; }
    fclose(f);
    long ndiff = 0;
    for (size_t i = 0; i < kPacked6Bytes; i++) ndiff += got[i] != gold[i];
    printf("  %s vs tinygrad: differ=%ld px\n", names[mi], ndiff);
    if (ndiff > (mi == 2 ? 0 : kMaxTieFlips)) { printf("FAIL %s 差异过多\n", names[mi]); fails++; }
  }
  printf(fails ? "warp golden: %d FAILURES\n" : "warp golden: ALL PASS\n", fails);
  return fails != 0;
}
