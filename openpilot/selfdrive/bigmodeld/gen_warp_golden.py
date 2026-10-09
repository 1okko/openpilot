#!/usr/bin/env python3
"""warp golden 生成（16 号 (d)）：用 lean modeld 同源的 get_warp_matrix
（openpilot/common/transformations/model.py:65-70）算出若干 rpyCalib 样本的
warp_road/warp_wide，输出 C++ 表，粘进 test_bigmodeld.cc 做 ≤1e-5 对照。

内参取 DEVICE_CAMERAS[('mici','os04c10')]（C4 的 os04c10 两路，1344×760），
并与 frame_meta.cpp 的 kNarrowRoadIntrinsics/kWideRoadIntrinsics 核对。

用法（1b-model-qnn 项目 venv 的 numpy；仓库根目录跑）：
  /Users/kevin/Documents/Projects/1b-model-qnn/.venv/bin/python \
      openpilot/selfdrive/bigmodeld/gen_warp_golden.py
"""
from __future__ import annotations

import os
import sys

import numpy as np

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
sys.path.insert(0, REPO_ROOT)

from openpilot.common.transformations.camera import DEVICE_CAMERAS  # noqa: E402
from openpilot.common.transformations.model import get_warp_matrix  # noqa: E402

# 样本 rpyCalib：零、典型标定残差、边界量级（rad）
SAMPLES = [
    (0.0, 0.0, 0.0),
    (0.01, -0.02, 0.005),
    (-0.03, 0.04, -0.01),
    (0.05, 0.05, 0.05),
    (-0.1, 0.0, 0.1),
    (0.0, -0.15, 0.0),
    (0.2, -0.2, 0.3),
    (1.0e-4, 2.0e-4, -3.0e-4),
]


def fmt(x: float) -> str:
    # float32 往返的 9 位有效数字；必须含小数点/指数才是合法 C++ 浮点字面量
    s = f"{np.float32(x).item():.9g}"
    if "." not in s and "e" not in s:
        s += ".0"
    return s + "f"


def main() -> int:
    dc = DEVICE_CAMERAS[("mici", "os04c10")]
    narrow = dc.narrow_road.intrinsics
    wide = dc.wide_road.intrinsics

    print("// gen_warp_golden.py 生成（勿手改）：rpy 样本 + get_warp_matrix 的 warp_road/warp_wide")
    print("// 数值口径：rpy 按 float32 取值（与 extrinsicsCalibration.rpyCalib 同）后以 double 精度")
    print("// 算同一公式（rot_from_euler + matmul + inv），与 frame_meta.cpp 的 double 实现对得上；")
    print("// modeld 实际路径的 float32 中间精度另带 ~3e-5 绝对噪声（相对 ~1e-7），不进本判据。")
    print(f"// 内参（DEVICE_CAMERAS[('mici','os04c10')]）：narrow fl={dc.narrow_road.focal_length} "
          f"wide fl={dc.wide_road.focal_length} size={dc.narrow_road.size}")
    print("struct WarpGolden { float rpy[3]; float road[9]; float wide[9]; };")
    print("static const WarpGolden kWarpGolden[] = {")
    for rpy in SAMPLES:
        # rpy 先落到 float32（wire/msgq 的实际精度）再提升 double，保证 C++/Python 输入逐位一致
        e = np.array([np.float32(v).item() for v in rpy], dtype=np.float64)
        road = get_warp_matrix(e, narrow, False).astype(np.float32).astype(np.float64).reshape(-1)
        wwide = get_warp_matrix(e, wide, True).astype(np.float32).astype(np.float64).reshape(-1)
        r = ", ".join(fmt(v) for v in e)
        rd = ", ".join(fmt(v) for v in road)
        wd = ", ".join(fmt(v) for v in wwide)
        print(f"  {{{{{r}}}, {{{rd}}}, {{{wd}}}}},")
    print("};")

    # 常量核对：frame_meta.cpp 的内参必须与 camera.py 一致
    print("// 内参核对（与 frame_meta.cpp 对读）：")
    for name, k in (("kNarrowRoadIntrinsics", narrow), ("kWideRoadIntrinsics", wide)):
        vals = ", ".join(fmt(v) for v in k.reshape(-1))
        print(f"//   {name}[9] = {{{vals}}}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
