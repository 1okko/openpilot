#!/usr/bin/env bash
export ATHENA_HOST='ws://athena.mr-one.cn'
export API_HOST='http://res.mr-one.cn'
# 强制车辆指纹（TOYOTA_SIENNA_PATCHED = Toyota Sienna 2021-26 PATCHED）
export FINGERPRINT=TOYOTA_SIENNA_PATCHED
# Skip onboarding on startup
echo -n "2" > /data/params/d/HasAcceptedTerms
echo -n "1.0" > /data/params/d/HasAcceptedTermsSP
echo -n "0.2.0" > /data/params/d/CompletedTrainingVersion
echo -n "1.0" > /data/params/d/CompletedSunnylinkConsentVersion  # Sunnylink 同意
echo -n "1" > /data/params/d/IsMetric
echo -n "1" > /data/params/d/ToyotaEnforceStockLongitudinal
# 强制 UI 语言为简体中文
echo -n "zh-CHS" > /data/params/d/LanguageSetting



# Force device timezone to Asia/Shanghai (Beijing)
sudo ln -sf /usr/share/zoneinfo/Asia/Shanghai /data/etc/localtime
sudo sh -c 'echo Asia/Shanghai > /data/etc/timezone'
export TZ="Asia/Shanghai"
exec ./launch_chffrplus.sh
