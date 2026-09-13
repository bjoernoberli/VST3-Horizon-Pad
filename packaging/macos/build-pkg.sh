#!/usr/bin/env bash
# Horizon Pad - macOS .pkg installer builder
#
# Builds an installer package that puts the plugin at the system-wide
# location (/Library/Audio/Plug-Ins/VST3/), matching the project's decision
# to install system-wide even though the build is unsigned/unnotarized.
# pkgbuild itself needs no admin rights to *build* the package - only
# *installing* it (writing under /Library) needs admin, which macOS's
# Installer app prompts for at install time.
#
# This produces an UNSIGNED package (no Apple Developer ID on this project -
# see README "Known gaps" / the project's distribution decisions). End users
# will see a Gatekeeper warning and need to right-click > Open, or allow it
# in System Settings > Privacy & Security, the first time they run it.
#
# Usage:
#   packaging/macos/build-pkg.sh "<path-to>/Horizon Pad.vst3" "<output-dir>"
#
# Requires: pkgbuild (ships with Xcode Command Line Tools / macOS itself -
# nothing extra to install on a Mac or a macOS CI runner).

set -euo pipefail

VST3_PATH="${1:?Usage: $0 <path-to-Horizon-Pad.vst3> <output-dir>}"
OUT_DIR="${2:?Usage: $0 <path-to-Horizon-Pad.vst3> <output-dir>}"

IDENTIFIER="com.quellemusic.horizonpad.installer"
VERSION="1.0.0"
INSTALL_LOCATION="/Library/Audio/Plug-Ins/VST3"
PKG_NAME="Horizon Pad.pkg"

if [ ! -d "$VST3_PATH" ]; then
    echo "ERROR: '$VST3_PATH' not found (expected the built Horizon Pad.vst3 bundle)." >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

# pkgbuild's --root mirrors --install-location: put the .vst3 in a staging
# root exactly as it should appear under /Library/Audio/Plug-Ins/VST3/.
STAGING_DIR="$(mktemp -d)"
COMPONENT_DIR="$(mktemp -d)"
trap 'rm -rf "$STAGING_DIR" "$COMPONENT_DIR"' EXIT

cp -R "$VST3_PATH" "$STAGING_DIR/"

# pkgbuild auto-analyzes any bundle under --root (a .vst3 is a bundle) and,
# by default, may flag it BundleIsRelocatable. That tells Installer.app to
# search the whole disk for anything sharing the plugin's bundle identifier
# and install there instead of at --install-location - so on a machine that
# already has a Horizon Pad.vst3 somewhere unexpected (an old manual copy,
# a different user's Library, etc.) the installer could silently write to
# the wrong place. A component plist pinning BundleIsRelocatable to false
# forces the install to always land exactly at --install-location.
COMPONENT_PLIST="$COMPONENT_DIR/component.plist"
pkgbuild --analyze --root "$STAGING_DIR" "$COMPONENT_PLIST"
/usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$COMPONENT_PLIST"

pkgbuild \
    --root "$STAGING_DIR" \
    --component-plist "$COMPONENT_PLIST" \
    --identifier "$IDENTIFIER" \
    --version "$VERSION" \
    --install-location "$INSTALL_LOCATION" \
    "$OUT_DIR/$PKG_NAME"

echo "Built: $OUT_DIR/$PKG_NAME"
