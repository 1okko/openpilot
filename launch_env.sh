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

# --- system time (this hardware has no battery-backed RTC) ---
# On a cold boot there is no sane clock: AGNOS restores a stale timestamp (or the epoch),
# which is still far earlier than the TLS certificate validity and makes the model
# list/download fail with "certificate is not yet valid". Force a known-recent date so
# TLS works immediately; NTP/GPS correct it afterwards.
if [ "$(date +%Y%m%d)" -lt 20261009 ]; then
  sudo date -s "2026-10-09 12:00:00" >/dev/null 2>&1
  printf 'seeded %s (was earlier than 2026-10-09)\n' "$(date -Is)" >> /data/time_seed.log 2>/dev/null
fi
