#!/usr/bin/env bash
# Builds dist/Ducktor-<version>-macos-<arch>.dmg from the ZIP that
# `cpack --preset release` already produced (run it first). The DMG contains
# Ducktor.app and a link to /Applications for drag-and-drop installation.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$ROOT/build/release"
DIST="$ROOT/dist"
STAGING="$ROOT/build/dmg"

VERSION="$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "$BUILD/CMakeCache.txt")"
ZIP="$(ls "$DIST"/Ducktor-"$VERSION"-macos-*.zip 2>/dev/null | head -n1)"
if [[ -z "$ZIP" ]]; then
    echo "No Ducktor-$VERSION-macos-*.zip in $DIST: run 'cpack --preset release' first." >&2
    exit 1
fi
DMG="${ZIP%.zip}.dmg"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
rm -rf "$STAGING"
mkdir -p "$STAGING"
# ditto preserves the ad hoc signature that the ZIP already carries.
ditto -x -k "$ZIP" "$TMP"
# CPack nests the payload under a top-level folder named after the ZIP, so
# locate Ducktor.app instead of assuming a fixed path.
APP="$(find "$TMP" -maxdepth 3 -type d -name 'Ducktor.app' | head -n1)"
if [[ -z "$APP" ]]; then
    echo "Ducktor.app not found inside $ZIP" >&2
    exit 1
fi
ditto "$APP" "$STAGING/Ducktor.app"
ln -s /Applications "$STAGING/Applications"

hdiutil create -volname "Ducktor" -srcfolder "$STAGING" -ov -format UDZO "$DMG"

echo "Created $DMG"
