#!/usr/bin/env bash
export ATHENA_HOST='ws://athena.mr-one.cn'
export API_HOST='http://res.mr-one.cn'
# Skip onboarding on startup
echo -n "2" > /data/params/d/HasAcceptedTerms
echo -n "1.0" > /data/params/d/HasAcceptedTermsSP
echo -n "0.2.0" > /data/params/d/CompletedTrainingVersion
echo -n "1.0" > /data/params/d/CompletedSunnylinkConsentVersion  # Sunnylink 同意
echo -n "1" > /data/params/d/IsMetric
echo -n "1" > /data/params/d/ToyotaEnforceStockLongitudinal
# 默认 UI 语言：简体中文（仅当尚未设置时写入，保留应用内的语言切换）
[ -f /data/params/d/LanguageSetting ] || echo -n "zh-CHS" > /data/params/d/LanguageSetting



# Force device timezone to Asia/Shanghai (Beijing)
sudo ln -sf /usr/share/zoneinfo/Asia/Shanghai /data/etc/localtime
sudo sh -c 'echo Asia/Shanghai > /data/etc/timezone'
export TZ="Asia/Shanghai"
exec ./launch_chffrplus.sh
