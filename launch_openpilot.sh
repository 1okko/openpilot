#!/usr/bin/env bash
# 强制车辆指纹（TOYOTA_SIENNA_PATCHED = Toyota Sienna 2021-26 PATCHED）
export FINGERPRINT=TOYOTA_SIENNA_PATCHED
echo -n "2" > /data/params/d/HasAcceptedTerms
echo -n "1.0" > /data/params/d/HasAcceptedTermsSP
echo -n "0.2.0" > /data/params/d/CompletedTrainingVersion
echo -n "1.0" > /data/params/d/CompletedSunnylinkConsentVersion  # Sunnylink 同意
echo -n "1" > /data/params/d/IsMetric
echo -n "1" > /data/params/d/ToyotaEnforceStockLongitudinal
echo -n "1" > /data/params/d/RoadEdgeLaneChangeEnabled
# 打灯变道默认不需要轻推（AutoLaneChangeTimer: 0=Nudge, 1=Nudgeless）
echo -n "1" > /data/params/d/AutoLaneChangeTimer

exec ./launch_chffrplus.sh
