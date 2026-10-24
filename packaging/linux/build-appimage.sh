#!/usr/bin/env bash
# Builds dist/Ducktor-<version>-x86_64.AppImage from the release preset.
# Extra arguments go to the CMake configure step, for example -DCMAKE_PREFIX_PATH=$HOME/Qt/6.10.0/gcc_64.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$ROOT/build/release"
APPDIR="$ROOT/build/AppDir"
TOOLS="$ROOT/build/appimage-tools"
DIST="$ROOT/dist"

LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_SHA256="c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d"
QT_PLUGIN_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/1-alpha-20250213-1/linuxdeploy-plugin-qt-x86_64.AppImage"
QT_PLUGIN_SHA256="15106be885c1c48a021198e7e1e9a48ce9d02a86dd0a1848f00bdbf3c1c92724"

fetch() {
    local url="$1" file="$2" sha="$3"
    if [[ ! -f "$file" ]]; then
        curl -fL --retry 3 -o "$file.part" "$url"
        mv "$file.part" "$file"
    fi
    echo "$sha  $file" | sha256sum --check --quiet
    chmod +x "$file"
}

cmake --preset release "$@"
cmake --build --preset release

rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD" --prefix /usr

mkdir -p "$TOOLS" "$DIST"
fetch "$LINUXDEPLOY_URL" "$TOOLS/linuxdeploy-x86_64.AppImage" "$LINUXDEPLOY_SHA256"
fetch "$QT_PLUGIN_URL" "$TOOLS/linuxdeploy-plugin-qt-x86_64.AppImage" "$QT_PLUGIN_SHA256"

VERSION="$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "$BUILD/CMakeCache.txt")"
# The Qt plugin locates Qt through qmake; with the Qt Online Installer set QMAKE=~/Qt/<version>/gcc_64/bin/qmake.
export QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake || true)}"
if [[ -z "$QMAKE" ]]; then
    echo "qmake not found: install qt6-base-dev-tools or set QMAKE to your Qt's bin/qmake." >&2
    exit 1
fi
export QML_SOURCES_PATHS="$ROOT/qml"
export EXTRA_QT_MODULES="svg"
export EXTRA_QT_PLUGINS="svg"
export LINUXDEPLOY_OUTPUT_VERSION="$VERSION"
export OUTPUT="$DIST/Ducktor-$VERSION-x86_64.AppImage"
# Lets the tools run on systems without FUSE 2.
export APPIMAGE_EXTRACT_AND_RUN=1

"$TOOLS/linuxdeploy-x86_64.AppImage" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/ducktor" \
    --desktop-file "$APPDIR/usr/share/applications/com.celticbytes.Ducktor.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/scalable/apps/com.celticbytes.Ducktor.svg" \
    --plugin qt \
    --output appimage

echo "Created $OUTPUT"
