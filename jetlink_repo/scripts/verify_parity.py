#!/usr/bin/env python3
"""
Copyright (c) 2026-, Zeph Leggett.

This file is part of jetlink and is licensed under the MIT License.
See the LICENSE file in the root directory for more details.

Does the link return the same numbers the model would?

Compares what comes back over the cable against onnxruntime on the unmodified
ONNX, per output slice and per column, so a regression lands on a named head
rather than in an 18452-wide vector. That covers the server's UINT8->FP16
preparation, the TensorRT build, the wire format and the output slicing. For a
queued graph, not the queues: reference() runs the same PolicyQueues, so a queue
bug cancels out on both sides, and tests/test_queues.py is the queue check. For
a stateful graph (openpilot #38916, Cinque Terre V3 on) reference() feeds each
next_state_ output back itself, so the server's state loop is checked too.

The graph is float16 end to end, so this is two float16 implementations
differing in accumulation order, not half against full precision: expect an
absolute 0.005 to 0.03 across the head values, and a few times that from a
phone GPU computing in float16 throughout. MIN_SAMPLES and QUIET_FRACTION say
what a correlation can judge from that; what it cannot is held to TINY_TOLERANCE.

    # 1. on the comma, over the cable (stop jetlinkd first, it owns the link).
    #    the server returns the spec of a model it already has, so only the
    #    model's identity is needed; --spec overrides it
    python3 scripts/verify_parity.py capture --ffs --dir out --sha256 <hex> --nbytes <n>
    #    or from a Mac standing in for the comma, with a phone dialing us
    python3 scripts/verify_parity.py capture --listen 5599 --dir out --sha256 <hex> --nbytes <n>

    # 2. anywhere with onnxruntime, on the ONNX the engine was built from
    python3 scripts/verify_parity.py reference --onnx big.onnx --dir out

    # 3. either machine
    python3 scripts/verify_parity.py compare --dir out
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np

from jetlink.spec import DRIVING_OUTPUT, ModelSpec

# Correlation, not an absolute tolerance: it moves on a wrong head, a transposed
# column or a stale queue, all of which a tolerance would wave through.
MIN_CORR = 0.999

# Correlation needs samples: one point correlates at 1.0 with anything. A slice or a
# column with fewer values a frame than this (lead_prob's three logits; pose, euler
# and road_transform, one value a column) is held to absolute error instead, which
# means something on any number of frames.
MIN_SAMPLES = 16

# Correlation needs spread: a column moving less than this fraction of its slice's
# spread (the plan's height and yaw rate) is float16 rounding against float16
# rounding. Held to absolute error too. LiteRT's float16 GPU path read 0.9988 on the
# plan's height and 0.992 on lead_prob while every head was within 6% of the
# reference; the noise floor (onnxruntime against itself) already read 0.9977 on
# road_transform's columns.
QUIET_FRACTION = 0.05

# What a slice or column held to absolute error may be off by: this fraction of its
# own largest reference value. The worst a float16 GPU showed on road-like frames was
# 6% (lead_prob), so a margin of 1.6; a negated, swapped or shifted head misses by
# 50% or more.
TINY_TOLERANCE = 0.1

# The tolerance's floor, as a fraction of the slice's spread: a column the model
# holds at ~0 (euler's roll, 1e-6 rad beside pitch and yaw of ~7) still gets the
# slack of float16 resolution, not a tenth of nothing.
CONSTANT_FRACTION = 1e-3

# How openpilot's Parser reads each head (parse_model_outputs.py): `hypotheses`
# blocks of [mu | std | selection], the last axis `columns` wide. Units mix on that
# axis, so a whole-slice correlation is set by the largest column; each is checked alone.
MDN_LAYOUTS = {
  'plan': [(0, 0, 15), (5, 1, 15)],
  'lane_lines': [(0, 0, 2)],
  'road_edges': [(0, 0, 2)],
  'lead': [(0, 0, 4), (2, 3, 4)],
  'pose': [(0, 0, 6)],
  'wide_from_device_euler': [(0, 0, 3)],
  'road_transform': [(0, 0, 6)],
  'sim_pose': [(0, 0, 6)],
}


def load_spec(args) -> ModelSpec | None:
  """--spec wins; otherwise the copy capture wrote next to the frames."""
  if args.spec:
    return ModelSpec.load(args.spec)
  path = Path(args.dir) / 'spec.json'
  return ModelSpec.load(path) if path.exists() else None


def make_inputs(spec: ModelSpec, n: int, seed: int = 0) -> list[tuple[np.ndarray, np.ndarray]]:
  """Deterministic stand-in frames, identical on both sides.

  Gradients and moving blobs rather than white noise, which drives a vision
  network into activations no road ever produces.
  """
  rng = np.random.default_rng(seed)
  h, w = spec.model_hw
  yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
  frames = []
  for i in range(n):
    warped = np.empty(spec.warped_shape, np.uint8)
    for cam in range(warped.shape[0]):
      for ch in range(warped.shape[1]):
        phase = 0.7 * i + 1.3 * ch + 2.1 * cam
        base = (96 + 64 * np.sin(xx / 37.0 + phase) + 48 * np.cos(yy / 23.0 - phase)
                + 24 * np.sin((xx + yy) / 61.0))
        warped[cam, ch] = np.clip(base + rng.normal(0, 6, (h, w)), 0, 255).astype(np.uint8)
    packed = np.zeros(spec.packed_nelem, np.float32)
    for name, (at, _) in spec.packed_layout.items():
      size = at.stop - at.start
      if name == 'traffic_convention':
        packed[at] = np.array([1.0, 0.0][:size])
      elif name == 'action_t':
        # Action horizons in seconds; modeld sends 0.2 to 0.4 s on a real car.
        # Bounded, not ramped: past ~1 s the plan runs backwards, out of distribution.
        packed[at] = np.array([0.25 + 0.1 * np.sin(0.5 * i), 0.35 + 0.1 * np.cos(0.5 * i)][:size])
      elif name == 'desire':
        # a pulse every eighth frame through the seven real desires; index 0 is
        # "none" and modeld zeroes it
        if i % 8 == 0:
          packed[at.start + 1 + (i // 8) % (size - 1)] = 1.0
    frames.append((warped, packed))
  return frames


# -- capture: what actually comes back over the link -------------------------

def capture(args) -> int:
  from jetlink.client import JetlinkClient

  spec = ModelSpec.load(args.spec) if args.spec else None
  if spec is not None:
    sha256, nbytes = spec.sha256, spec.nbytes
  elif args.sha256 and args.nbytes:
    sha256, nbytes = args.sha256, args.nbytes
  else:
    raise SystemExit("capture needs --spec, or --sha256 and --nbytes of a model the server already has")
  out = Path(args.dir)
  out.mkdir(parents=True, exist_ok=True)

  # the whole output, hidden_state included: it is most of the vector, and the
  # server's own feedback of it is part of what this checks
  if args.ffs:
    client = JetlinkClient.open_ffs(args.ffs_mount, gadget=args.gadget, want_hidden=True)
  elif args.host:
    client = JetlinkClient.open_tcp(args.host, args.port, want_hidden=True)
  else:
    print(f"waiting up to {args.listen_timeout:.0f}s for a peer to dial {args.listen}...")
    client = JetlinkClient.open_listen(args.listen, args.listen_timeout, want_hidden=True)

  try:
    hello = client.hello(timeout=60.0)  # the jetson may still be re-enumerating
    print(f"server: {hello.get('backend', 'trt')} {hello.get('runtime_version', hello.get('trt_version'))} "
          f"on {hello['device']}")
    # a model the server does not have is a build job, not a parity check
    spec = client.ensure_engine(sha256, nbytes, build_timeout=300.0)
    # reference and compare need the slices this capture was made with
    (out / 'spec.json').write_text(json.dumps(spec.to_dict()))
    frames = make_inputs(spec, args.n, args.seed)

    for i, (warped, packed) in enumerate(frames):
      # the server feeds each frame's hidden state into the next, as modeld did
      result = client.infer(warped, packed, frame_id=i, reset=(i == 0))
      np.save(out / f'in_warped_{i}.npy', warped)
      np.save(out / f'in_packed_{i}.npy', packed)
      np.save(out / f'out_link_{i}.npy', np.asarray(result, np.float32))
      print(f"  frame {i}: {len(result)} values, "
            f"finite={bool(np.all(np.isfinite(result)))}")
  finally:
    client.close()
  print(f"wrote {args.n} frames to {out}")
  return 0


# -- reference: the same inputs through onnxruntime ---------------------------

_ORT_DTYPES = {
  'tensor(uint8)': np.uint8,
  'tensor(int32)': np.int32,
  'tensor(int64)': np.int64,
  'tensor(float16)': np.float16,
  'tensor(float)': np.float32,
  'tensor(double)': np.float64,
  'tensor(bool)': np.bool_,
}


def _ort_feed_dtypes(sess) -> dict[str, np.dtype]:
  """What the graph declares for each input, not a guess about which need casting."""
  dtypes = {}
  for i in sess.get_inputs():
    if i.type not in _ORT_DTYPES:
      raise SystemExit(f"input {i.name} is {i.type}; add it to _ORT_DTYPES")
    dtypes[i.name] = _ORT_DTYPES[i.type]
  return dtypes


def reference(args) -> int:
  import onnxruntime as ort

  from jetlink.queues import PolicyQueues

  spec = load_spec(args)
  if spec is None:
    from jetlink.spec import spec_from_onnx
    spec = spec_from_onnx(args.onnx)
  d = Path(args.dir)

  sess = ort.InferenceSession(args.onnx, providers=['CPUExecutionProvider'])
  n = len(sorted(d.glob('in_warped_*.npy')))
  if not n:
    raise SystemExit(f"no captured inputs in {d}; run capture first")
  if spec.stateful:
    return reference_stateful(spec, sess, d, n)

  # The untouched ONNX still wants UINT8 images where the queues hand back FP16.
  # 0..255 is exact in both, so the cast is lossless.
  dtypes = _ort_feed_dtypes(sess)
  queues = PolicyQueues(spec)
  queues.reset()

  for i in range(n):
    warped = np.load(d / f'in_warped_{i}.npy')
    packed = np.load(d / f'in_packed_{i}.npy')
    feed = queues.step(warped, packed)
    missing = set(dtypes) - set(feed)
    if missing:
      raise SystemExit(f"the queues produced no value for graph input(s) {sorted(missing)}")
    feed = {k: np.ascontiguousarray(v, dtype=dtypes[k]) for k, v in feed.items()}
    out = np.asarray(sess.run(None, feed)[0], np.float32).reshape(-1)
    np.save(d / f'out_ref_{i}.npy', out)
    print(f"  frame {i}: {out.shape[0]} values, finite={bool(np.all(np.isfinite(out)))}")
    # the hidden state fed back is our own previous output, as the server's
    # is its own; the link's would hide the drift this is looking for
    queues.after_run({DRIVING_OUTPUT: out})
  return 0


def reference_stateful(spec: ModelSpec, sess, d: Path, n: int) -> int:
  """A stateful graph run the way openpilot's ModelState runs it: the frame and
  the scalars in, each next_state_ output fed back as its state_ input, and
  every state zero at the start as after the capture's reset."""
  dtypes = _ort_feed_dtypes(sess)
  shapes = {i.name: tuple(i.shape) for i in sess.get_inputs()}
  names = [o.name for o in sess.get_outputs()]
  state = {name: np.zeros(shapes[name], dtypes[name]) for name in spec.state_pairs}
  for i in range(n):
    warped = np.load(d / f'in_warped_{i}.npy')
    packed = np.load(d / f'in_packed_{i}.npy')
    # a stateful graph's packed names are its own input names
    feed = {'new_img': warped, **state,
            **{name: packed[at] for name, (at, _) in spec.packed_layout.items()}}
    missing = set(dtypes) - set(feed)
    if missing:
      raise SystemExit(f"no value for graph input(s) {sorted(missing)}")
    feed = {k: np.ascontiguousarray(v, dtype=dtypes[k]).reshape(shapes[k]) for k, v in feed.items()}
    outs = dict(zip(names, sess.run(None, feed), strict=True))
    out = np.asarray(outs[DRIVING_OUTPUT], np.float32).reshape(-1)
    np.save(d / f'out_ref_{i}.npy', out)
    print(f"  frame {i}: {out.shape[0]} values, finite={bool(np.all(np.isfinite(out)))}")
    state = {name: outs[nxt] for name, nxt in spec.state_pairs.items()}
  return 0


# -- compare ------------------------------------------------------------------

def _corr(a: np.ndarray, b: np.ndarray) -> float:
  if a.std() == 0 and b.std() == 0:
    # a head the model holds constant is a pass, not a division by zero
    return 1.0
  if a.std() == 0 or b.std() == 0:
    return 1.0 if np.allclose(a, b) else 0.0
  return float(np.corrcoef(a, b)[0, 1])


def columns(name: str, a: np.ndarray) -> dict[str, np.ndarray] | None:
  """Split a raw head into unit-homogeneous columns, or None if its layout is unknown."""
  for hyp, sel, width in MDN_LAYOUTS.get(name, ()):
    rows = max(hyp, 1)
    if a.size % rows:
      continue
    raw = a.reshape(rows, -1)
    n = (raw.shape[1] - sel) // 2
    if n <= 0 or (raw.shape[1] - sel) % 2 or n % width:
      continue
    mu = raw[:, :n].reshape(-1, width)
    std = raw[:, n:2 * n].reshape(-1, width)
    cols = {f'mu[{j}]': mu[:, j] for j in range(width)}
    cols.update({f'std[{j}]': std[:, j] for j in range(width)})
    if sel:
      cols['sel'] = raw[:, 2 * n:].reshape(-1)
    return cols
  return None


def _frames(x) -> list[np.ndarray]:
  """One array or a list of per-frame arrays, as flat float32 frames."""
  seq = x if isinstance(x, (list, tuple)) else [x]
  return [np.asarray(f, np.float32).reshape(-1) for f in seq]


def tolerance(ref: np.ndarray, spread: float) -> float:
  """What a slice or column held to absolute error may be off by: TINY_TOLERANCE of
  its largest reference value, and never less than CONSTANT_FRACTION of `spread`,
  its slice's standard deviation."""
  return max(TINY_TOLERANCE * float(np.abs(ref).max(initial=0.0)), CONSTANT_FRACTION * spread)


def judge(a: np.ndarray, b: np.ndarray, per_frame: int, spread: float | None = None) -> tuple[bool, str]:
  """Whether `a` matches the reference `b`, and the number that says so. Correlation,
  unless there is too little to correlate: fewer than MIN_SAMPLES values a frame, or
  (a column, `spread` its slice's) moving less than QUIET_FRACTION of its slice. Those
  are held to absolute error, within `tolerance` of their own largest value."""
  if per_frame < MIN_SAMPLES or (spread is not None and b.std() < QUIET_FRACTION * spread):
    bound = tolerance(b, b.std() if spread is None else spread)
    err = np.abs(a - b).max()
    return err <= bound, f'by error, max abs {err:.4g} {"<=" if err <= bound else ">"} {bound:.4g}'
  c = _corr(a, b)
  return c >= MIN_CORR, f'{c:.6f}'


def report_slices(spec: ModelSpec, links, refs) -> dict[str, bool]:
  """One line per output slice with every frame pooled. Returns whether each passed.

  A slice passes when its pooled correlation and every column's clear MIN_CORR. A
  slice or column with fewer than MIN_SAMPLES values a frame, or a column moving less
  than QUIET_FRACTION of its slice, is held to absolute error instead (`tolerance`),
  because correlating it is rounding noise against rounding noise.
  """
  links, refs = _frames(links), _frames(refs)
  passed = {}
  for name, sl in sorted(spec.output_slices.items()):
    a = np.concatenate([x[sl] for x in links])
    b = np.concatenate([y[sl] for y in refs])
    whole = _corr(a, b)
    # a slice's own size, not stop - start: a spec may count from the end, pad as [-2:]
    ok, why = judge(a, b, links[0][sl].size)
    detail = f"{why if why.startswith('by error') else '(compared whole)':38}"
    cols_a = [columns(name, x[sl]) for x in links]
    if cols_a[0]:
      cols_b = [columns(name, y[sl]) for y in refs]
      worst_c, worst_k, failed, by_error = 2.0, '', [], 0
      for k in cols_a[0]:
        ca = np.concatenate([c[k] for c in cols_a])
        cb = np.concatenate([c[k] for c in cols_b])
        col_ok, col_why = judge(ca, cb, cols_a[0][k].size, b.std())
        if col_why.startswith('by error'):
          by_error += 1
        elif float(col_why) < worst_c:
          worst_c, worst_k = float(col_why), k
        if not col_ok:
          failed.append(f'{k} {col_why}')
      ok &= not failed
      worst = f"worst col {worst_c:8.6f} {worst_k:8}" if worst_k else f"{'no column correlated':27}"
      note = f'({by_error} by error)' if by_error else ''
      detail = f"{worst} {note:14}"
      if failed:
        detail += '  cols: ' + ', '.join(failed)
    passed[name] = ok
    flag = '' if ok else '   <-- FAIL'
    print(f"    {name:24} corr {whole:8.6f}  {detail}  max abs {np.abs(a - b).max():8.4f}  "
          f"mean abs {np.abs(a - b).mean():7.5f}{flag}")
  return passed


def compare(args) -> int:
  spec = load_spec(args)
  if spec is None:
    raise SystemExit(f"no spec: pass --spec, or run capture first (it writes {Path(args.dir) / 'spec.json'})")
  d = Path(args.dir)
  n = len(sorted(d.glob('out_link_*.npy')))
  if not n:
    raise SystemExit(f"no captured outputs in {d}")

  links, refs = [], []
  for i in range(n):
    link = np.load(d / f'out_link_{i}.npy').reshape(-1)
    ref_path = d / f'out_ref_{i}.npy'
    if not ref_path.exists():
      raise SystemExit(f"missing {ref_path}; run reference first")
    ref = np.load(ref_path).reshape(-1)
    m = min(len(link), len(ref))
    links.append(link[:m])
    refs.append(ref[:m])

  # Per frame: a stale queue or a dropped reset shows on the frame it happens to.
  # A slice too small to correlate is held to error, which the pooled check below
  # applies to every frame at once.
  frame_fail: dict[str, float] = {}
  for i, (link, ref) in enumerate(zip(links, refs, strict=True)):
    print(f"\nframe {i}: corr {_corr(link, ref):.6f}  max abs {np.abs(link - ref).max():.4f}")
    for name, sl in sorted(spec.output_slices.items()):
      a, b = link[sl], ref[sl]
      c = _corr(a, b)
      gated = a.size >= MIN_SAMPLES
      if gated and c < MIN_CORR:
        frame_fail[name] = min(frame_fail.get(name, 2.0), c)
      flag = '   <-- FAIL' if gated and c < MIN_CORR else ('' if gated else '   (by error, pooled)')
      print(f"    {name:24} corr {c:8.6f}  max abs {np.abs(a - b).max():8.4f}  "
            f"mean abs {np.abs(a - b).mean():7.5f}{flag}")

  print(f"\npooled over {n} frames, per slice and per column:")
  passed = report_slices(spec, links, refs)

  bad = [f'{k} {v:.6f} on one frame' for k, v in sorted(frame_fail.items(), key=lambda kv: kv[1])]
  bad += [f'{k} pooled' for k, ok in passed.items() if not ok and k not in frame_fail]
  if bad:
    print(f"\nFAIL: {len(bad)} slice(s) below corr {MIN_CORR} or past their error bound, whole or in a column: "
          + ', '.join(bad))
    return 1
  print(f"\nOK: every slice and every column at or above corr {MIN_CORR}, or within {TINY_TOLERANCE:.0%} of its "
        f"largest value where too small or too quiet to correlate, per frame and pooled over all {n} frames")
  return 0


def main() -> int:
  p = argparse.ArgumentParser(description=__doc__,
                              formatter_class=argparse.RawDescriptionHelpFormatter)
  p.add_argument('mode', choices=('capture', 'reference', 'compare'))
  p.add_argument('--spec', help='model spec json; overrides the one capture writes to --dir')
  p.add_argument('--sha256', help='capture mode: model identity, when the server already has it')
  p.add_argument('--nbytes', type=int, help='capture mode: ONNX size in bytes, with --sha256')
  p.add_argument('--dir', default='parity')
  p.add_argument('--n', type=int, default=32,
                 help='capture mode: frames; more frames give the correlations more to go on')
  p.add_argument('--seed', type=int, default=0)
  p.add_argument('--onnx', help='reference mode: the ONNX the engine was built from')
  p.add_argument('--ffs', action='store_true', help='capture mode: this end is the gadget')
  p.add_argument('--ffs-mount', default='/dev/ffs-jetlink')
  p.add_argument('--gadget', default='/sys/kernel/config/usb_gadget/jetlink')
  p.add_argument('--host', help='capture mode: the server over TCP')
  p.add_argument('--port', type=int, default=5599)
  p.add_argument('--listen', metavar='[HOST:]PORT',
                 help='capture mode: accept one incoming dial (a phone over the cable network '
                      'dials the comma; from a Mac this stands in for it)')
  p.add_argument('--listen-timeout', type=float, default=120.0, metavar='SECONDS',
                 help='--listen: how long to wait for the dial')
  args = p.parse_args()

  if args.mode == 'capture' and sum(map(bool, (args.ffs, args.host, args.listen))) != 1:
    p.error('capture takes one of --ffs, --host and --listen')

  if args.mode == 'reference' and not args.onnx:
    raise SystemExit('reference mode needs --onnx')
  return {'capture': capture, 'reference': reference, 'compare': compare}[args.mode](args)


if __name__ == '__main__':
  sys.exit(main())
