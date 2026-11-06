#!/usr/bin/env bash
# Builds dist/Ducktor-<version>-macos-<arch>.pkg from the ZIP that
# `cpack --preset release` already produced (run it first). The package
# installs the ad hoc signed Ducktor.app into /Applications.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$ROOT/build/release"
DIST="$ROOT/dist"
STAGING="$ROOT/build/pkg"

VERSION="$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "$BUILD/CMakeCache.txt")"
ZIP="$(ls "$DIST"/Ducktor-"$VERSION"-macos-*.zip 2>/dev/null | head -n1)"
if [[ -z "$ZIP" ]]; then
    echo "No Ducktor-$VERSION-macos-*.zip in $DIST: run 'cpack --preset release' first." >&2
    exit 1
fi
PKG="${ZIP%.zip}.pkg"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
sudo rm -rf "$STAGING"
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
# The installer keeps the ownership recorded in the payload, so hand it over
# to root: an app installed from a CI build must not belong to the build user.
sudo chown -R 0:0 "$STAGING"

pkgbuild --root "$STAGING" --identifier com.celticbytes.Ducktor \
    --version "$VERSION" --install-location /Applications "$PKG"

echo "Created $PKG"
