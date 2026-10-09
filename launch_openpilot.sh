#!/usr/bin/env bash
export ATHENA_HOST='ws://athena.mr-one.cn'
export API_HOST='http://res.mr-one.cn'
# Skip onboarding on startup
echo -n "2" > /data/params/d/HasAcceptedTerms
echo -n "1.0" > /data/params/d/HasAcceptedTermsSP
echo -n "0.2.0" > /data/params/d/CompletedTrainingVersion
echo -n "1.0" > /data/params/d/CompletedSunnylinkConsentVersion  # Sunnylink 同意
echo -n "1" > /data/params/d/IsMetric




# --- system time ---------------------------------------------------------------
# This hardware has no battery-backed RTC: every boot starts at 1970, so TLS fails
# ("certificate is not yet valid") and the model list/download cannot succeed until
# NTP or GPS catches up. Restore the last known time from /data (persists across
# reboots) so TLS works right away; NTP/GPS correct it to the second afterwards.
TIME_SNAPSHOT=/data/time_snapshot
if [ -f "$TIME_SNAPSHOT" ]; then
  SAVED_TIME=$(cat "$TIME_SNAPSHOT" 2>/dev/null || echo 0)
  NOW_TIME=$(date +%s)
  # only when the clock is bogus (before 2020) and the snapshot looks sane (after 2020)
  if [ "$NOW_TIME" -lt 1577836800 ] && [ "${SAVED_TIME:-0}" -gt 1577836800 ]; then
    sudo date -s "@$SAVED_TIME" >/dev/null 2>&1 && echo "[time] restored from snapshot: $(date)"
  fi
fi

# keep the snapshot fresh so the next boot starts close to the real time
(
  while true; do
    date +%s > "$TIME_SNAPSHOT.tmp" 2>/dev/null && mv "$TIME_SNAPSHOT.tmp" "$TIME_SNAPSHOT" 2>/dev/null
    sleep 300
  done
) &

# Force device timezone to Asia/Shanghai (Beijing)
sudo ln -sf /usr/share/zoneinfo/Asia/Shanghai /data/etc/localtime
sudo sh -c 'echo Asia/Shanghai > /data/etc/timezone'
export TZ="Asia/Shanghai"
exec ./launch_chffrplus.sh
