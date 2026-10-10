#!/usr/bin/env bash
#
# Copyright (c) 2026-, Zeph Leggett.
# This file is part of jetlink and is licensed under the MIT License.
#
# Push this package to a comma for testing. The owner builds the USB gadget
# itself, on its first step.
#
# The repo lands at <openpilot>/jetlink_repo with a symlink <openpilot>/jetlink
# into its package dir, as launch_chffrplus.sh does for tinygrad and opendbc.
#
#   scripts/deploy_to_comma.sh comma@192.168.1.143
#
# The openpilot updater's reset --hard + clean deletes untracked files, so set
# DisableUpdates=1 on the device while testing.
set -euo pipefail

HOST="${1:?usage: deploy_to_comma.sh user@host [dest]}"
DEST="${2:-/data/openpilot/jetlink_repo}"

here="$(cd "$(dirname "$0")/.." && pwd)"
root="$(dirname "$DEST")"

# This jetlink keeps none of the names a fork from before its jetlink adapter
# (openpilot/sunnypilot/accelerators) calls: there, manager's should_run for
# jetlinkd raises and manager exits on every boot, link on or off.
if ! ssh "$HOST" "test -f '$root/openpilot/sunnypilot/jetlink_adapter/__init__.py'"; then
  echo "!! $HOST:$root has no openpilot/sunnypilot/jetlink_adapter: that fork predates the adapter," >&2
  echo "   and this jetlink would stop its manager. Update the fork first, or deploy an older jetlink." >&2
  exit 1
fi

echo "==> syncing $here -> $HOST:$DEST"
rsync -a --delete \
  --exclude '.git' --exclude '__pycache__' --exclude '.pytest_cache' \
  --exclude 'tests' --exclude 'docker' \
  --exclude 'JetlinkKit' --exclude 'ios' --exclude 'macos' --exclude 'plans' \
  --exclude '.build' --exclude 'build' --exclude '*.egg-info' \
  "$here/" "$HOST:$DEST/"

echo "==> linking $root/jetlink -> $(basename "$DEST")/jetlink"
# ln -sfn refuses to replace a real directory, so clear one an older install left
ssh "$HOST" "[ -d '$root/jetlink' ] && [ ! -L '$root/jetlink' ] && rm -rf '$root/jetlink'; \
             ln -sfn '$(basename "$DEST")/jetlink' '$root/jetlink'"

echo "==> checking the package imports under the AGNOS venv"
ssh "$HOST" "cd '$root' && PYTHONPATH='$root' /usr/local/venv/bin/python3 -c '
import jetlink, jetlink.client, jetlink.transport.ffs, jetlink.queues, jetlink.openpilot.owner
print(\"jetlink\", jetlink.__version__, \"ok\")'"

cat <<'NEXT'

==> done. The owner, the one resident jetlink process on the comma, builds the
    USB gadget on its first step, for USB or iOS as the Jetlink setting says, and
    binds it. The provisioning run is what it starts when there is work, and
    that exits.

    An owner or modeld that was already running still has the OLD package
    imported. Reboot the comma, or kill the owner: a fork with jetlinkd's
    restart wrapper starts it again on manager's next loop (an older fork
    leaves it dead until a reboot), and three kills in 10 min make its next
    start wait. Get the pid first: pkill -f over ssh matches your own ssh
    command line and kills the session:

      pgrep -f "^openpilot.sunnypilot.jetlink_adapter$"

    Sanity check with the Jetson cabled up and its server running
    (the jetlink-server service, or jetlink run):

      ssh <comma> 'cat /sys/class/udc/*/state'      # want: configured
      ssh <jetson> 'lsusb -d 1209:0001'             # want: the gadget listed
NEXT
