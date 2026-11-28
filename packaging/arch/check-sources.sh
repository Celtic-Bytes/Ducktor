#!/usr/bin/env bash
# Regression check for the Arch package sources.
#
# Verifies that:
#   1. The PKGBUILD's source list is intact — makepkg --verifysource downloads
#      the tag tarball (into a temp dir) and checks the pinned SHA-256. Local
#      (non-URL) sources are resolved with `makepkg --printsrcinfo` — which
#      parses and evaluates the PKGBUILD — never by hand-parsing the source
#      array (which would break on single quotes, variables, multiline arrays
#      or nontrivial local paths). Relative local sources inside packaging/arch/
#      are staged next to the PKGBUILD in the temp directory and hashed against
#      sha256sums (e.g. a legitimate local backport of another vintage).
#      makepkg resolves local sources by basename in the build directory, so
#      staging copies each relative source to its basename; a basename clash
#      between two local sources is rejected. Absolute paths and paths that
#      escape packaging/arch/ are not supported — tarball-first is the
#      deliberate scope.
#      This is POST-TAG ONLY: the tagged PKGBUILD carries a stale own-tarball
#      checksum by design (the tarball contains the PKGBUILD itself), so
#      running this before the corrected checksum is committed (release
#      procedure step 4) fails by design.
#   2. The tag carries the reviewed fixes upstream and no obsolete backport:
#      - the tag's cmake/Engines.cmake must reference the pinned NuGet
#        package for ONNX Runtime (the reviewed b1d24eb fix) — if it does not,
#        the tarball cannot build;
#      - the obsolete v0.3.0 backport patch must not be present — neither the
#        file nor any reference to it in the PKGBUILD (dead history; a local
#        patch of any other vintage is a legitimate source, staged and
#        verified by makepkg, never banned here).
#
# makepkg refuses to run as root, so run this as a non-root user (as the
# release workflow does for the post-tag validation). It is the repeatable
# check behind the release procedure's post-tag steps.
set -euo pipefail

cd "$(dirname "$0")"
REPO_DIR="$(pwd)"
PKGBUILD_FILE="$REPO_DIR/PKGBUILD"
PKGVER="$(sed -n 's/^pkgver=//p' "$PKGBUILD_FILE")"

TMPDIR_CHECK="$(mktemp -d)"
trap 'rm -rf "$TMPDIR_CHECK"' EXIT

mkdir -p "$TMPDIR_CHECK/work" "$TMPDIR_CHECK/downloads"
cp "$PKGBUILD_FILE" "$TMPDIR_CHECK/work/PKGBUILD"

# Resolve the full source list with makepkg itself — it parses and evaluates
# the PKGBUILD (single vs double quotes, variables, multiline arrays, local
# paths). Redirecting through a temp file keeps this robust in non-tty and
# pipe contexts; --printsrcinfo fails loudly on a malformed PKGBUILD.
echo "==> Resolving the PKGBUILD source list"
if ! (cd "$TMPDIR_CHECK/work" && makepkg --printsrcinfo > "$TMPDIR_CHECK/printsrcinfo.log" 2>"$TMPDIR_CHECK/printsrcinfo.err"); then
    cat "$TMPDIR_CHECK/printsrcinfo.err" >&2
    echo "    ERROR: makepkg --printsrcinfo failed — the PKGBUILD is broken" >&2
    exit 1
fi
# --printsrcinfo tab-indents the property lines after the first, so allow
# leading whitespace before the "source =" prefix.
mapfile -t SOURCES < <(sed -n 's/^[[:space:]]*source = //p' "$TMPDIR_CHECK/printsrcinfo.log")
if [[ ${#SOURCES[@]} -eq 0 ]]; then
    echo "    ERROR: makepkg --printsrcinfo reported no sources" >&2
    exit 1
fi

# Stage every relative local (non-URL) source inside packaging/arch/ next to
# the PKGBUILD so makepkg --verifysource can hash it against sha256sums.
# makepkg looks local sources up by basename in the build directory, so each
# source is staged as its basename; a basename clash is a real error. A missing
# or outside-the-repo source is a real error, not a silent skip.
declare -A STAGED=()
for src in "${SOURCES[@]}"; do
    if [[ "$src" == *://* ]]; then
        continue    # remote URL: makepkg downloads it into SRCDEST
    fi
    if [[ "$src" == /* || "$src" == ..* || "$src" == */*..* || "$src" == */*/.. ]]; then
        echo "    ERROR: local source '$src' is outside packaging/arch/ — only relative local sources inside packaging/arch/ are supported" >&2
        exit 1
    fi
    if [[ ! -f "$REPO_DIR/$src" ]]; then
        echo "    ERROR: local source '$src' is missing from packaging/arch/" >&2
        exit 1
    fi
    base="$(basename "$src")"
    if [[ -n "${STAGED[$base]+x}" ]]; then
        echo "    ERROR: local sources '${STAGED[$base]}' and '$src' share the basename '$base' — only one local source per basename is supported" >&2
        exit 1
    fi
    STAGED[$base]="$src"
    cp "$REPO_DIR/$src" "$TMPDIR_CHECK/work/$base"
    echo "==> staging local source: $src (as $base)"
done

echo "==> Verifying the PKGBUILD sources"
(cd "$TMPDIR_CHECK/work" && env SRCDEST="$TMPDIR_CHECK/downloads" makepkg --verifysource)

tar -xzf "$TMPDIR_CHECK/downloads/v${PKGVER}.tar.gz" -C "$TMPDIR_CHECK"
SRC_DIR="$TMPDIR_CHECK/Ducktor-${PKGVER}"
if [[ ! -d "$SRC_DIR" ]]; then
    echo "    ERROR: the tarball did not extract to Ducktor-${PKGVER}/" >&2
    exit 1
fi

echo "==> Checking v${PKGVER} carries the NuGet fix upstream and no obsolete backport"
if ! grep -q "nuget.org/api/v2/package/Microsoft.ML.OnnxRuntime" "$SRC_DIR/cmake/Engines.cmake"; then
    echo "    ERROR: v${PKGVER} has no NuGet fix upstream — the tarball cannot build" >&2
    echo "    the engines. Cut a new version from a fixed main instead." >&2
    exit 1
fi
if [[ -f "$REPO_DIR/v0.3.0-onnxruntime-nuget.patch" ]] \
    || grep -qF "v0.3.0-onnxruntime-nuget.patch" "$PKGBUILD_FILE"; then
    echo "    ERROR: the obsolete v0.3.0 backport patch is still present in" >&2
    echo "    packaging/arch/ or the PKGBUILD — remove the file, its source and" >&2
    echo "    sha256sums entries and any prepare() guard." >&2
    exit 1
fi
echo "==> OK: v${PKGVER} carries the reviewed fix upstream; the obsolete"
echo "==> v0.3.0 backport is gone"

echo "==> Source regression check passed for pkgver ${PKGVER}"
