#!/bin/bash
# Builds the VST3 (if needed) and installs it to the system plugin folder.
#
# Root can't read the built bundle directly when the project lives on an
# OneDrive-synced path, so this always stages a copy in /tmp first (as the
# regular user) and only elevates for the /tmp -> /Library copy.
set -euo pipefail

cd "$(dirname "$0")/.."

cmake --build build --target HorizonPad_VST3 -j 8

SRC="build/HorizonPad_artefacts/Debug/VST3/Horizon Pad.vst3"
STAGE="/tmp/HorizonPadInstall"
DEST="/Library/Audio/Plug-Ins/VST3/Horizon Pad.vst3"

rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -R "$SRC" "$STAGE/"

osascript -e "do shell script \"rm -rf '$DEST' && cp -R '$STAGE/Horizon Pad.vst3' '$DEST'\" with administrator privileges"

echo "Installed to $DEST"
