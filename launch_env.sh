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
