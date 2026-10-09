#!/usr/bin/env bash

export OMP_NUM_THREADS=1
export MKL_NUM_THREADS=1
export NUMEXPR_NUM_THREADS=1
export OPENBLAS_NUM_THREADS=1
export VECLIB_MAXIMUM_THREADS=1

# models get lower priority than ui
# - ui is ~5ms
# - modeld is 20ms
# - DM is 10ms
# in order to run ui at 60fps (16.67ms), we need to allow
# it to preempt the model workloads. we have enough
# headroom for this until ui is moved to the CPU.
export QCOM_PRIORITY=12

if [ -z "$AGNOS_VERSION" ]; then
  export AGNOS_VERSION="19.7-c3xl-dev"
fi

export STAGING_ROOT="/data/safe_staging"

# Comma4-UI-Streamer (MJPEG UI stream) - set STREAM=0 to disable
export STREAM=1
export STREAM_PORT=8082
export STREAM_QUALITY=50
export STREAM_FPS=10

# --- system time (this hardware has no battery-backed RTC) ---
# Every boot the clock starts at 1970, which breaks TLS: the model list and downloads fail
# with "certificate is not yet valid" until NTP or GPS catches up. Seed a recent date so TLS
# works from the start; NTP/GPS correct it to the second afterwards.
if [ "$(date +%Y)" -lt 2020 ]; then
  sudo date -s "2026-10-09 12:00:00" >/dev/null 2>&1
  printf 'seeded %s (was < 2020)\n' "$(date -Is)" >> /data/time_seed.log 2>/dev/null
fi
