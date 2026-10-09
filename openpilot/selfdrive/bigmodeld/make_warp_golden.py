#!/usr/bin/env python3
"""生成 warp golden 参考：bigmodeld/fixtures/warp_ref_m{1,2,3}.bin（test_warp_golden.cc 对照）。

参考 = lean-master compile_modeld.make_frame_prepare 的真 tinygrad 实现（21 号 ref_check 同法）。
输入 NV12 与 test_warp_golden.cc 的 LCG 公式同式（两端字节一致），矩阵同 21 号 MATS（m3 为 .5
tie 压力矩阵）。每矩阵一帧 packed6（196608 B）。tinygrad 后端经环境变量选择（DEV=CPU 强制 CPU，
默认走 Mac DEFAULT_DEVICE），生成一次入库；验收口径见 test_warp_golden.cc。

用法：python3 openpilot/selfdrive/bigmodeld/make_warp_golden.py
依赖：numpy + zstandard + tinygrad 源码树（见 TINYGRAD）。
"""
import importlib.util
import os
import sys

import numpy as np

TINYGRAD = '/Users/kevin/Documents/Projects/tinygrad'
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
LM = os.path.join(REPO, 'openpilot/selfdrive/modeld/compile_modeld.py')
W, H, MW, MH = 1344, 760, 512, 256
PW, PH = MW // 2, MH // 2

MATS = {
    'm1': np.array([[2.6, 0.05, 120], [0.02, 2.6, -20], [3e-5, -1e-5, 1]], dtype=np.float32),
    'm2': np.array([[-2.5, 0.1, 900], [0.03, -2.4, 600], [2e-5, 3e-5, 1]], dtype=np.float32),
    'm3': np.array([[2.5, 0.0, 0.5], [0.0, 2.5, 0.5], [0.0, 0.0, 1]], dtype=np.float32),
}


def lcg_frame() -> np.ndarray:
    """与 test_warp_golden.cc 的 lcg_frame 同式（uint32 LCG，取 s>>24）。"""
    n = W * H * 3 // 2
    s = 0x9E3779B9
    out = np.empty(n, dtype=np.uint8)
    for i in range(n):
        s = (s * 1664525 + 1013904223) & 0xFFFFFFFF
        out[i] = s >> 24
    return out.reshape(H * 3 // 2, W)


def load_lm():
    sys.path.insert(0, TINYGRAD)
    sys.path.insert(0, REPO)  # `openpilot` 包根
    spec = importlib.util.spec_from_file_location('lm_compile', LM)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def main():
    outdir = os.path.join(HERE, 'fixtures')
    os.makedirs(outdir, exist_ok=True)
    frame = lcg_frame()

    mod = load_lm()
    from tinygrad.device import Device
    from tinygrad.tensor import Tensor
    print(f'tinygrad DEFAULT_DEVICE={Device.DEFAULT} (DEV env={os.environ.get("DEV", "<default>")})')

    nv = mod.NV12Frame(width=W, height=H, stride=W, y_height=H, uv_height=H // 2,
                       size=W * H * 3 // 2)
    fp = mod.make_frame_prepare(nv, MW, MH)

    for name, M in MATS.items():
        got = fp(Tensor(frame.reshape(-1)), Tensor(M)).numpy()
        assert got.shape == (6, PH, PW) and got.dtype == np.uint8, got.shape
        path = os.path.join(outdir, f'warp_ref_{name}.bin')
        got.astype(np.uint8).tofile(path)
        print(f'wrote {path} ({got.nbytes} bytes)')


if __name__ == '__main__':
    main()
