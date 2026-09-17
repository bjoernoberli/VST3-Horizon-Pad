#!/bin/bash
# Builds the VST3 (if needed) and installs it to the CURRENT USER's plugin
# folder (~/Library/Audio/Plug-Ins/VST3), which every major DAW (including
# Ableton Live) scans right alongside the system-wide one. This needs no
# elevated privileges at all - no osascript, no admin password prompt - so
# it can run completely unattended (e.g. from a background agent iterating
# on its own). See the one-time migration note in git history for why this
# replaced the old /Library + osascript version.
set -euo pipefail

cd "$(dirname "$0")/.."

cmake --build build --target HorizonPad_VST3 -j 8

SRC="build/HorizonPad_artefacts/Debug/VST3/Horizon Pad.vst3"
DEST="$HOME/Library/Audio/Plug-Ins/VST3/Horizon Pad.vst3"

mkdir -p "$(dirname "$DEST")"
rm -rf "$DEST"
cp -R "$SRC" "$DEST"

echo "Installed to $DEST"
