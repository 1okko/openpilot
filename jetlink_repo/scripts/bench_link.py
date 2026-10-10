#!/usr/bin/env python3
"""
Copyright (c) 2026-, Zeph Leggett.

This file is part of jetlink and is licensed under the MIT License.
See the LICENSE file in the root directory for more details.

Measure the round trip the car depends on.

Jitter is the question, not bandwidth: modeld has 50 ms a frame and the model
takes ~21 ms of it. Sends real-sized payloads at the real rate and reports the tail.

    # against a Jetson on the LAN
    python3 scripts/bench_link.py --host 192.168.1.87 --onnx big_model.onnx --n 400

    # over the cable, from the comma (the comma is the gadget). the server
    # returns the spec of a model it already has, so its identity is enough
    python3 scripts/bench_link.py --ffs --sha256 <hex> --nbytes <n>
    python3 scripts/bench_link.py --ffs --spec spec.json

    # the same with jetlinkd running: borrow the link from it, as modeld does
    python3 scripts/bench_link.py --ffs --loan --spec spec.json

    # from a Mac standing in for the comma: wait for one phone to dial us
    # (the app dials whatever answers on its cable network), then bench it
    python3 scripts/bench_link.py --listen 5599 --onnx big_model.onnx
"""
from __future__ import annotations

import argparse
import logging
import sys
import time
from pathlib import Path

import numpy as np

from jetlink.client import JetlinkClient
from jetlink.spec import ModelSpec, spec_from_onnx

# modeld's per-frame budget; a frame past it is dropped, and frameDropPerc > 1 soft-disables
FRAME_BUDGET_MS = 50.0
# how long a frame sent as datagrams is waited for before it counts as held
DATAGRAM_HOLD = 1.0
# how long --loan waits for the gadget owner to lend the link
LOAN_TIMEOUT = 60.0


def load_spec(args) -> ModelSpec | None:
  """Take the spec from a file or the model itself; None leaves it to the server."""
  if args.spec:
    return ModelSpec.load(args.spec)
  if args.onnx:
    return spec_from_onnx(args.onnx)
  return None


def _wait_for_host(timeout: float) -> None:
  """Block until a USB host has configured us.

  Opening the transport is what binds the gadget, so the wait belongs after the
  open: until a host attaches every read just times out.
  """
  udcs = list(Path('/sys/class/udc').glob('*/state'))
  deadline = time.monotonic() + timeout
  reported = False
  while time.monotonic() < deadline:
    for udc in udcs:
      try:
        if udc.read_text().strip() == 'configured':
          print(f"host attached ({udc.parent.name})")
          return
      except OSError:
        pass
    if not reported:
      print(f"waiting up to {timeout:.0f}s for a USB host to enumerate the gadget...")
      reported = True
    time.sleep(0.25)
  raise SystemExit("no USB host attached: check the cable")


def pct(a: np.ndarray, q: float) -> float:
  return float(np.percentile(a, q))


def open_loan(timeout: float = LOAN_TIMEOUT):
  """Borrow the link from the comma's gadget owner and open it as modeld
  does. (loan, client); close the client, then the loan."""
  from jetlink.comma import lending
  loan = lending.borrow('bench', timeout=timeout)
  if loan is None:
    raise SystemExit(f"no loan from the gadget owner in {timeout:g}s: is jetlinkd running, "
                     "and is modeld or jetlink_hold.py holding it?")
  print(f"borrowed the {'cable link' if loan.cable else 'gadget'}: udc {loan.udc}, mount {loan.mount}")
  try:
    from jetlink.openpilot.link import connect
    return loan, connect(logging.getLogger('jetlink.bench'), loan, name='bench', wait=timeout)
  except BaseException:
    loan.close()
    raise


def main() -> int:
  p = argparse.ArgumentParser()
  g = p.add_mutually_exclusive_group(required=True)
  g.add_argument('--host', help='TCP host of the Jetson')
  g.add_argument('--ffs', action='store_true',
                 help='this end is the USB gadget (FunctionFS) - use this on a comma')
  g.add_argument('--listen', metavar='[HOST:]PORT',
                 help='accept one incoming dial (a phone over the cable network dials the comma; '
                      'from a Mac this stands in for it) and bench over that connection')
  p.add_argument('--listen-timeout', type=float, default=120.0, metavar='SECONDS',
                 help='--listen: how long to wait for the dial')
  p.add_argument('--port', type=int, default=5599)
  p.add_argument('--loan', action='store_true',
                 help="with --ffs: borrow the link from the comma's gadget owner (jetlinkd), as modeld does, "
                      "rather than opening the gadget")
  p.add_argument('--ffs-mount', default='/dev/ffs-jetlink')
  p.add_argument('--gadget', default='/sys/kernel/config/usb_gadget/jetlink')
  p.add_argument('--wait-host', type=float, default=0.0, metavar='SECONDS',
                 help='gadget mode: wait for a host to enumerate us before starting')
  p.add_argument('--spec', help='json spec file, as `jetlink-server spec ONNX` prints it')
  p.add_argument('--onnx', help='read the spec from this model, uploading it if the server lacks it')
  p.add_argument('--sha256', help='model identity, for a model the server already has')
  p.add_argument('--nbytes', type=int, help='ONNX size in bytes, with --sha256')
  p.add_argument('--n', type=int, default=400)
  p.add_argument('--no-datagrams', action='store_true',
                 help='keep frames on the stream where a phone\'s server offers datagrams, for an A/B')
  p.add_argument('--rate', type=float, default=20.0, help='Hz; 0 = as fast as possible')
  args = p.parse_args()
  if args.loan and not args.ffs:
    p.error('--loan goes with --ffs')

  loan = None
  if args.loan:
    loan, client = open_loan()
  elif args.ffs:
    # opening this writes the descriptors and binds the UDC, so the Jetson can enumerate us
    client = JetlinkClient.open_ffs(args.ffs_mount, gadget=args.gadget)
  elif args.listen:
    print(f"waiting up to {args.listen_timeout:.0f}s for a peer to dial {args.listen}...")
    client = JetlinkClient.open_listen(args.listen, args.listen_timeout)
  else:
    client = JetlinkClient.open_tcp(args.host, args.port)
  try:
    return _run(args, client)
  finally:
    # a FunctionFS owner that dies leaves the gadget bound with nothing servicing it
    client.close()
    if loan is not None:
      loan.close()   # the owner takes the endpoints back


def _run(args, client) -> int:
  if args.wait_host:
    _wait_for_host(args.wait_host)

  client.allow_datagrams = not args.no_datagrams
  hello = client.hello()
  print(f"server: {hello.get('backend', 'trt')} {hello.get('runtime_version', hello.get('trt_version'))} "
        f"on {hello['device']}, engine {hello['engine_state']}")

  spec = load_spec(args)
  if spec is not None:
    sha256, nbytes = spec.sha256, spec.nbytes
  elif args.sha256 and args.nbytes:
    sha256, nbytes = args.sha256, args.nbytes
  else:
    raise SystemExit("need --spec, --onnx, or --sha256 and --nbytes of a model the server already has")
  t0 = time.time()
  spec = client.ensure_engine(sha256, nbytes, onnx_path=args.onnx,
                              progress=lambda s, f, m: print(f"  {s:<7} {f*100:5.1f}%  {m}"))
  print(f"engine ready in {time.time() - t0:.1f}s")

  rng = np.random.default_rng(0)
  warped = rng.integers(0, 256, spec.warped_shape, dtype=np.uint8)
  packed = np.zeros(spec.packed_nelem, np.float32)

  lat, gpu, queue, srv = [], [], [], []
  held = 0
  send_ms, recv_ms = [], []
  period = 1.0 / args.rate if args.rate > 0 else 0.0
  next_t = time.perf_counter()
  for i in range(args.n):
    if period:
      now = time.perf_counter()
      if next_t > now:
        time.sleep(next_t - now)
      next_t += period
    t = time.perf_counter()
    # as modeld sends them: over the cable, a frame after the first may go as
    # datagrams, and one that never comes back is held, not a dead link
    lose = i > 0 and client.t.datagrams
    seq = client.infer_begin(warped, packed, frame_id=i, reset=(i == 0), skip_if_busy=lose)
    t_sent = time.perf_counter()
    if client.infer_end(seq, hold=DATAGRAM_HOLD if lose else None) is None:
      held += 1
      continue
    t_done = time.perf_counter()
    lat.append((t_done - t) * 1e3)
    send_ms.append((t_sent - t) * 1e3)
    recv_ms.append((t_done - t_sent) * 1e3)
    g_us, q_us, t_us = client.last_timings
    gpu.append(g_us / 1e3)
    queue.append(q_us / 1e3)
    srv.append(t_us / 1e3)

  a = np.array(lat[10:])
  print(f"\npayload: {spec.infer_req_nbytes/1e3:.0f} KB up, "
        f"{spec.infer_resp_nbytes/1e3:.0f} KB down, "
        f"{(spec.infer_req_nbytes+spec.infer_resp_nbytes)*args.rate*8/1e6:.0f} Mbit/s at {args.rate:g} Hz")
  print(f"round trip over {len(a)} frames (ms):")
  print(f"  mean {a.mean():6.2f}  min {a.min():6.2f}  p50 {pct(a,50):6.2f}  "
        f"p90 {pct(a,90):6.2f}  p99 {pct(a,99):6.2f}  max {a.max():6.2f}")
  print(f"  jitter: p99-p50 {pct(a,99)-pct(a,50):5.2f}  stdev {a.std():5.2f}")
  # a USB write returns once the host has taken the data, so send is wire time there;
  # a TCP send is a memcpy and the whole wire cost lands in recv
  snd, rcv = np.array(send_ms[10:]), np.array(recv_ms[10:])
  if args.host or args.listen:
    print("  split (TCP): send is the copy into the socket buffer; both directions of wire time are in recv")
  else:
    print("  split (USB): the write blocks until the host has read, so send is request wire time, "
          "recv is server + reply")
  print(f"  send ({spec.infer_req_nbytes/1e3:.0f} KB up):   mean {snd.mean():6.2f}  p50 {pct(snd,50):6.2f}  max {snd.max():6.2f}")
  print(f"  recv ({spec.infer_resp_nbytes/1e3:.0f} KB down): mean {rcv.mean():6.2f}  p50 {pct(rcv,50):6.2f}  max {rcv.max():6.2f}")
  s = np.array(srv[10:])
  print(f"server-side total {np.mean(s):6.2f} ms  (gpu {np.mean(gpu[10:]):5.2f}, "
        f"queues {np.mean(queue[10:]):5.2f})")
  print(f"transport overhead: {a.mean() - s.mean():.2f} ms mean")
  over = int((a > FRAME_BUDGET_MS).sum())
  print(f"frames over the {FRAME_BUDGET_MS:.0f} ms budget: {over}/{len(a)} ({100*over/len(a):.1f}%)")
  print(f"frames went {'as datagrams' if client.t.datagrams else 'on the stream'}; "
        f"{held} held past {DATAGRAM_HOLD:g} s, {client.frames_lost} never answered")
  return 0


if __name__ == '__main__':
  sys.exit(main())
