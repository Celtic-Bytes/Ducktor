#!/usr/bin/env bash
# Regression check for the Arch package sources.
#
# Verifies that:
#   1. The PKGBUILD's source array is intact — makepkg --verifysource downloads
#      the tag tarball (into a temp dir) and checks the pinned SHA-256 of the
#      tarball and of the local backport patch. This is the genuine checksum
#      verification. It is POST-TAG ONLY: the tagged PKGBUILD carries a stale
#      own-tarball checksum by design (the tarball contains the PKGBUILD
#      itself), so running this before the corrected checksum is committed
#      (release procedure step 4) fails by design.
#   2. The backport patch is consistent with the tag:
#      - pkgver == 0.3.0: the tag must NOT carry the NuGet fix upstream (if
#        it does, the backport is stale) and the backport patch must apply
#        to the tag's exact sources.
#      - pkgver != 0.3.0: the tag must carry the NuGet fix upstream and the
#        backport must be gone — no patch file left behind and no patch
#        reference in the PKGBUILD. Failing here is the signal that a
#        backport was never removed after the release that carries the fix.
#
# makepkg refuses to run as root, so run this as a non-root user (as the
# release workflow does for the post-tag validation). Run it locally after
# bumping pkgver, after changing the patch, or after the corrected checksum
# is committed — it is the repeatable check behind the prepare() guard in
# the PKGBUILD.
set -euo pipefail

cd "$(dirname "$0")"
REPO_DIR="$(pwd)"
PKGBUILD_FILE="$REPO_DIR/PKGBUILD"
PKGVER="$(sed -n 's/^pkgver=//p' "$PKGBUILD_FILE")"
PATCH="v0.3.0-onnxruntime-nuget.patch"

TMPDIR_CHECK="$(mktemp -d)"
trap 'rm -rf "$TMPDIR_CHECK"' EXIT

echo "==> Verifying the PKGBUILD sources"
mkdir -p "$TMPDIR_CHECK/work" "$TMPDIR_CHECK/downloads"
cp "$PKGBUILD_FILE" "$TMPDIR_CHECK/work/PKGBUILD"
if [[ "$PKGVER" == "0.3.0" ]]; then
    cp "$REPO_DIR/$PATCH" "$TMPDIR_CHECK/work/$PATCH"
fi
(cd "$TMPDIR_CHECK/work" && env SRCDEST="$TMPDIR_CHECK/downloads" makepkg --verifysource)

tar -xzf "$TMPDIR_CHECK/downloads/v${PKGVER}.tar.gz" -C "$TMPDIR_CHECK"
SRC_DIR="$TMPDIR_CHECK/Ducktor-${PKGVER}"
if [[ ! -d "$SRC_DIR" ]]; then
    echo "    ERROR: the tarball did not extract to Ducktor-${PKGVER}/" >&2
    exit 1
fi

if [[ "$PKGVER" == "0.3.0" ]]; then
    echo "==> Checking the v0.3.0 backport patch applies to the tag sources"
    if grep -q "nuget.org/api/v2/package/Microsoft.ML.OnnxRuntime" "$SRC_DIR/cmake/Engines.cmake"; then
        echo "    ERROR: the v0.3.0 tag already carries the NuGet fix upstream;" >&2
        echo "    drop the backport patch, its sha256sums entry and the prepare() guard." >&2
        exit 1
    fi
    (cd "$SRC_DIR" && patch -p1 --dry-run < "$REPO_DIR/$PATCH")
    echo "==> OK: patch applies cleanly to the v0.3.0 sources"
else
    echo "==> Checking v${PKGVER} carries the NuGet fix upstream and no backport"
    if ! grep -q "nuget.org/api/v2/package/Microsoft.ML.OnnxRuntime" "$SRC_DIR/cmake/Engines.cmake"; then
        echo "    ERROR: v${PKGVER} has no NuGet fix upstream — the v0.3.0-" >&2
        echo "    specific backport does not apply to this tag. Restore the build" >&2
        echo "    for this tag (or add a matching backport) before cutting it." >&2
        exit 1
    fi
    if [[ -f "$REPO_DIR/$PATCH" ]] || grep -q "v0.3.0-onnxruntime-nuget.patch" "$PKGBUILD_FILE"; then
        echo "    ERROR: the v0.3.0 backport is still present in a release whose" >&2
        echo "    tag carries the fix upstream — remove the patch file, its" >&2
        echo "    sha256sums entry, the prepare() guard and 'patch' in makedepends." >&2
        exit 1
    fi
    echo "==> OK: v${PKGVER} carries the reviewed fix upstream; no backport remains"
fi

echo "==> Source regression check passed for pkgver ${PKGVER}"
