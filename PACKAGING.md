# Packaging

Where Ducktor's packages are built, where they are installed, and how to
diagnose and fix the common failure modes. This file is the deep reference for
packaging work; the [README](README.md) keeps the quick-start with the exact
commands per format. Keep both in sync when packaging changes (see
[Keeping this document updated](#keeping-this-document-updated)).

Package outputs go to `dist/` (the Arch package lands next to the PKGBUILD in
`packaging/arch/`; the CI release workflow also carries the Flatpak bundle
through its artifact, out of the workspace root). No package includes AI
models: users download those from _Settings_ after installing.

## Build and install layout

Install layout lives in [cmake/Install.cmake](cmake/Install.cmake) and must
match the search paths in [src/core/Paths.cpp](src/core/Paths.cpp). Do not
change one without the other.

At runtime the app finds its engines by searching, in order: the app directory
(`ducktor` on Linux, the app bundle or the executable directory on Windows and
macOS), `../libexec/ducktor` (the `.deb`/RPM/tar.xz layout), and
`DUCKTOR_ENGINE_DIR`. Bundled spell data and the `espeak-ng-data` folder are
looked up the same way (plus `share/ducktor/*` and `Resources/*` as relative
candidates). Bundled models are looked up in `models/` next to the app,
`share/ducktor/models`, `Resources/models` and `DUCKTOR_MODELS_DIR`; no package
bundles a model.

| Platform    | Application                     | Engines                                                                          | Docs/licenses (next to the app or in `/usr/share/doc/ducktor`) | Other |
| ---------- | ------------------------------- | --------------------------------------------------------------------------------- | ---------------------------------------------------------------- | ---- |
| Linux      | `/usr/bin/ducktor`              | `/usr/libexec/ducktor/` — the four helpers, plus `spell/`, `espeak-ng-data/` and the Piper/ONNX Runtime libraries                     | `/usr/share/doc/ducktor/` — `LICENSE`, `PRIVACY.md`, `THIRD_PARTY_NOTICES.md` and a `licenses/` folder | Desktop file, metainfo and icons under `/usr/share/` |
| Windows    | `ducktor.exe` (everything next to it) | `ducktor-*.exe`, Qt DLLs (by `windeployqt`) and the Microsoft C++ runtime next to `ducktor.exe`; `spell/`, `espeak-ng-data/` and the Piper/ONNX Runtime DLLs | Next to `ducktor.exe` | `portable.txt` makes settings/models live in a `data/` folder next to the app |
| macOS      | `Ducktor.app/Contents/MacOS/ducktor`     | Engines (`ducktor-*`), `spell/`, `espeak-ng-data/` and the Piper/ONNX Runtime libraries in `Ducktor.app/Contents/MacOS/`, Qt by `macdeployqt` | Next to `Ducktor.app` | Ad hoc signed inside the install step (`Install.cmake`) |

The app links Qt; the engines do not — none of them uses a Qt package (verified
in `engines/*/CMakeLists.txt`), they are plain C++ helpers started by the app,
each as a separate helper process so a crash or OOM in one kills only that
helper. On Linux the `.deb`/RPM/tar.xz use the system Qt (they depend on the
distribution's `qt6-*` packages and do not bundle it); the AppImage carries
its own Qt (via `linuxdeploy-plugin-qt`); the Flatpak builds against the KDE
6.10 runtime; Windows and macOS carry their Qt inside the package.

## Package formats and how they are produced

| Format | Produced by | Output (local) |
| ------ | ----------- | -------------- |
| `.deb` | `cmake --workflow --preset package` (the `release` preset) | `dist/ducktor_<version>_amd64.deb` |
| `.tar.xz` | same workflow as `.deb` | `dist/ducktor-<version>-linux-x86_64.tar.xz` |
| RPM | `cmake --workflow --preset package-rpm` (the `release-rpm` preset, in a Fedora container) | `dist/ducktor-<version>-1.x86_64.rpm` |
| AppImage | `packaging/linux/build-appimage.sh` | `dist/Ducktor-<version>-x86_64.AppImage` |
| Flatpak | `flatpak-builder` + `flatpak build-bundle` with `packaging/flatpak/com.celticbytes.Ducktor.yml` | `dist/Ducktor-<version>.flatpak` (CI uploads the bundle from the workspace root) |
| Windows ZIP | `cmake --preset release` + `cpack --preset release` on Windows | `dist/Ducktor-<version>-windows-x64.zip` |
| macOS ZIP | same as Windows, on macOS | `dist/Ducktor-<version>-macos-<arch>.zip` |
| macOS DMG | `packaging/macos/build-dmg.sh` after `cpack` | `dist/Ducktor-<version>-macos-<arch>.dmg` |
| macOS pkg | `packaging/macos/build-pkg.sh` after `cpack` (installs to `/Applications`) | `dist/Ducktor-<version>-macos-<arch>.pkg` |
| Arch (AUR) | `makepkg` with `packaging/arch/PKGBUILD` (from the tag tarball; needs network) | `ducktor-<pkgver>-<pkgrel>-x86_64.pkg.tar.zst` next to the PKGBUILD |

Rules that apply to every format:

- Each system's packages must be built on that system (Linux on Linux, the
  Windows ZIP on Windows, the macOS ZIP on a Mac).
- The version comes from `project(Ducktor VERSION ...)` in
  [CMakeLists.txt](CMakeLists.txt); the release dates and notes for the Linux
  packages live in the `releases` block of
  [packaging/linux/com.celticbytes.Ducktor.metainfo.xml](packaging/linux/com.celticbytes.Ducktor.metainfo.xml).
- Every package ships the license texts: `LICENSE`, `PRIVACY.md`,
  `THIRD_PARTY_NOTICES.md` and the `licenses/` folder — populated by the
  `install(...)` rules in [cmake/Install.cmake](cmake/Install.cmake) from the
  license texts copied out of each engine during the build (see
  [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for the full list). The
  spell dictionaries also keep their own license texts next to the dictionary
  data (`spell/licenses/`, shipped with the engine directory). No package ships
  a model.

## Pinned sources and integrity checks

Pinned downloads split into two categories — do not describe git-tag pins as
hash-verified:

| Component | Version pin | How it is verified |
| -------- | ----------- | ------------------ |
| llama.cpp (translation engine) | `v0.7.0` | git tag + shallow clone (`engines/translate/CMakeLists.txt`); no SHA-256 |
| whisper.cpp (speech recognition) | `v1.9.5` | git tag + shallow clone (`engines/transcribe/CMakeLists.txt`); no SHA-256 |
| Piper libpiper (speech synthesis) | `v1.8.0` | git tag + shallow clone (`cmake/Engines.cmake`); no SHA-256 |
| Hunspell (spell check) | `v1.7.6` | git tag + shallow clone (`engines/spellcheck/CMakeLists.txt`); no SHA-256 |
| ONNX Runtime | `1.22.0` (NuGet `Microsoft.ML.OnnxRuntime`) | `URL_HASH SHA256=d571e63a2329baacb713f441e65ad75284de354db6e1ac435fe4bebbb417986a` (`cmake/Engines.cmake`) |
| linuxdeploy / linuxdeploy-plugin-qt (AppImage) | `1-alpha-20251107-1` / `1-alpha-20250213-1` | SHA-256 in `packaging/linux/build-appimage.sh` |
| Arch PKGBUILD source | tag tarball `v<pkgver>.tar.gz` | `sha256sums` in the PKGBUILD (see below) |
| Arch v0.3.0 backport patch (historical — removed with the release that followed the v0.3.0 backport) | exact reviewed diff `b446eae..b1d24eb` (NuGet fix) — the file is gone; preserved in the git history (`git show 511a147:packaging/arch/v0.3.0-onnxruntime-nuget.patch`) | not part of the current tree; see [v0.3.0 backport](#v030-backport-arch-package) |

The engines (and piper's nested espeak-ng) are built as separate CMake projects
via `ExternalProject` in [cmake/Engines.cmake](cmake/Engines.cmake); llama.cpp
and whisper.cpp both vendor ggml, so they are never `add_subdirectory`'d into
the same project. Pinned engines need network access during the build; that is
expected.

## v0.3.0 backport (Arch package)

> **Historical record — removed with the release that followed the v0.3.0
> backport.** This section documents the v0.3.0 backport repair, which is
> **past**: the release whose sources carry the reviewed fix upstream removed
> the patch file, its `sha256sums` entry and the `prepare()` guard, and
> `check-sources.sh` no longer looks for the patch. It is kept for the record
> — the v0.3.0 tag is still a broken release whose sources come from a tree
> that predates the fix.

The v0.3.0 tag (`b446eae`) predates the reviewed NuGet fix (`b1d24eb`): its
`cmake/Engines.cmake` still downloads ONNX Runtime from the removed
`microsoft/onnxruntime` GitHub release, so `makepkg` fails while configuring
libpiper (see [Troubleshooting](#onnx-runtime-download-fails-every-package-build-fails-while-configuring-piper)).
The v0.3.0 tag itself is left untouched — moving a release tag to fix a
build is off the table.

The v0.3.0 Arch package was repaired by backporting the reviewed fix to the
tag via a build-time patch (removed from the release whose sources carry the
fix upstream):

- The patch was the exact reviewed diff
  (`git diff b446eae b1d24eb -- cmake/Engines.cmake engines/speak/CMakeLists.txt`),
  checked in as `packaging/arch/v0.3.0-onnxruntime-nuget.patch` while the
  backport was live. The file no longer exists in this tree; the exact
  reviewed diff is preserved in the git history at
  `git show 511a147:packaging/arch/v0.3.0-onnxruntime-nuget.patch`.
- It was a pinned source: listed in `source` and hashed in `sha256sums`
  (`30db300dc0087832fa111a2eec0ded270fe654f0a9e9dad8526f614481ddf341`), so a
  tampered or truncated patch failed `makepkg --verifysource` before the
  build starts.
- The v0.3.0 PKGBUILD's `prepare()` applied it with `patch -p1 --forward`,
  guarded on `pkgver == "0.3.0"`: later tags carry the fix upstream and must
  not get the patch (the tarball already has it; applying it would fail or be a
  no-op).
- The patch must be distributed alongside the PKGBUILD (e.g. in the AUR git
  repository) — it is not part of the tarball.

Repeatable regression check: [packaging/arch/check-sources.sh](packaging/arch/check-sources.sh).
It is **post-tag use**: the tagged PKGBUILD carries the stale own-tarball
checksum by design (the tarball contains the PKGBUILD itself), so
`--verifysource` fails on the tag ref by design. The release workflow runs
the check as the build user **only on a manual dispatch with a full build
requested**, before the build, and refuses the dispatch from a tag ref (see
`.github/workflows/releases.yml`, "Verify the Arch sources (post-tag)"). On a
tag push only the metadata/consistency validation runs — never the checksum.
The script checks the tag: it resolves the `source` list with
`makepkg --printsrcinfo`, stages any relative local sources it can support
(inside `packaging/arch/`, staged by basename) next to the PKGBUILD in the
temp directory, verifies the tarball checksum with `makepkg --verifysource`,
requires the NuGet URL in the tag's `cmake/Engines.cmake`, and fails if the
obsolete v0.3.0 backport patch reappears — no file, no PKGBUILD reference.
Local source patches of other vintages are **legitimate sources** of a release
(e.g. a future pinned backport): they are staged and hash-verified against the
recorded `sha256sums` — never banned. The v0.3.0 backport itself was never
re-introduced: the release carrying the fix upstream removed the local patch
entirely.

Maintenance: the first release prepared from a tree that carries the fix
upstream removed the patch file, its `sha256sums` entry, the `prepare()` guard
and `patch` from `makedepends` (together with the `prepare()` function itself).
If the v0.3.0 backport patch reappears, `check-sources.sh` fails — the obsolete
backport is never a legitimate source of a release whose tree carries the fix
upstream. A local patch of another vintage (e.g. a future pinned backport) is a
legitimate source; it is staged and hash-verified by `makepkg --verifysource`.

The packaged v0.3.0 was also re-packaged as **0.3.0-2** (`pkgrel=2`) by this
backport: same source tag, fixed build — bumping `pkgrel` is the Arch
convention for a re-packaged build of the same version, so downstream can tell
the fixed package apart from the broken 0.3.0-1.

### Verified result (v0.3.0 backport, Nov 2026)

Full makepkg build in a CI-faithful environment (`archlinux/archlinux:latest`
container, `base-devel`, `builder` user, `makepkg --syncdeps --noconfirm`,
matching the workflow's Arch job; the build was capped at four cores via
`taskset -c 0-3` to emulate the CI runner's 4 vCPUs):

- The tag tarball and the backport patch pass `makepkg --verifysource`
  (both print `Passed`).
- `prepare()` applies the patch to the tag's sources
  (`patching file cmake/Engines.cmake`, `patching file engines/speak/CMakeLists.txt`).
- The full build completes: `ducktor-0.3.0-1-x86_64.pkg.tar.zst`
  (plus `ducktor-debug-...`) is created and `makepkg` exits 0. After the
  `pkgrel=2` bump, `makepkg --repackage --force --noconfirm` reuses the
  compiled files to create `ducktor-0.3.0-2-x86_64.pkg.tar.zst`.
- Package contents (untarred): `ducktor` + four engines in
  `/usr/libexec/ducktor/`, `libpiper.so` + `libonnxruntime.so.1` next to the
  helpers, `espeak-ng-data/` (402 files) and `spell/` dictionaries with their
  licenses, `/usr/share/doc/ducktor/` (LICENSE, PRIVACY.md,
  THIRD_PARTY_NOTICES.md, `licenses/` per engine), desktop file, metainfo,
  icons. **No** AI models (`.onnx`/`.gguf`/`.mmproj`) are bundled.
- Smoke test: `ducktor-speak` and `ducktor-translate` load their
  shared-library dependencies and print the protocol's `bad_arguments` error
  on empty argv; `ducktor-speak` resolves `libpiper.so` and
  `libonnxruntime.so.1` relative to its own directory.
- Known warnings (cosmetic, not build failures): makepkg notes a
  `$srcdir` reference in the `.BUILDINFO` for the bundled libraries and
  `gdb-add-index` finds no debugging symbols in `libonnxruntime.so.1`.

## Release procedure (tagged releases)

**Pre-tag checks** (steps 1–2): no tarball exists yet for the new tag, so
all that can be checked is metadata and consistency. The `sha256sums` recorded
in the PKGBUILD is stale by design (the tarball contains the PKGBUILD itself,
so its hash can never match the commit that creates it). A pre-tag tree carries
the **provisional** hash of the previous tag's tarball, recorded so that step 4
has a value to replace; it is wrong on purpose, it is never a reason to skip or
bypass the check, and it must be replaced with the real v<version> tarball's
hash in step 4.

1. Bump `project(Ducktor VERSION ...)` in [CMakeLists.txt](CMakeLists.txt) and
   add the new entry to [CHANGELOG.md](CHANGELOG.md) (and to the `releases`
   block of the metainfo file). Commit.
2. Bump `pkgver` (and `pkgrel=1`) in [packaging/arch/PKGBUILD](packaging/arch/PKGBUILD). Commit.
   Do **not** try to make `sha256sums` correct at this step — the checksum of
   the tag's tarball is only knowable once the tag exists. The `sha256sums`
   array has a single entry (the tarball) plus any local source files — the
   first release prepared from a tree carrying the fix upstream removed the
   v0.3.0 backport patch; a future release that adds one backs itself up the
   same way (see [v0.3.0 backport](#v030-backport-arch-package) for the
   historical example).

   ```bash
   (cd packaging/arch && makepkg --printsrcinfo)   # valid metadata
   ```

**At tag time** (step 3): `git tag v<version> && git push origin v<version>`.
The workflow's `check-version` job fails the release if the tag does not
match the project version in CMakeLists.txt (push triggers only; manual
runs skip it). The Arch job additionally validates the PKGBUILD `pkgver` and,
on a tag push, that the tag's tarball declares the same project version; on a
manual dispatch it validates `pkgver` against the version in CMakeLists.txt.
The tarball checksum is **not** checked here — see the structural constraint
below.

**Post-tag checks** (steps 4–5): the checksum is knowable, so this is the
moment to verify it and recalculate it:

4. Download the tag's tarball and recalculate the checksum. Do this from the
   post-tag state of the tree (the tag itself stays where it is — the tag
   never moves). The `sha256sums` array has one entry — the tarball — so
   replacing the first hex string in it replaces the tarball's hash:

   ```bash
   PKGVER="$(sed -n 's/^pkgver=//p' packaging/arch/PKGBUILD)"   # run from the repo root
   curl -fL --retry 3 -o /tmp/ducktor-src.tar.gz \
       "https://github.com/Celtic-Bytes/Ducktor/archive/refs/tags/v${PKGVER}.tar.gz"
   TARBALL_HASH="$(sha256sum /tmp/ducktor-src.tar.gz | cut -d' ' -f1)"
   git rev-parse "v${PKGVER}^{commit}"       # sanity: verify the tag did not move
   sed -i "s|^sha256sums=('[0-9a-f]\{64\}'|sha256sums=('${TARBALL_HASH}'|" packaging/arch/PKGBUILD
   # Verify the one-entry layout: the single entry is the tarball's hash.
   sed -n '/^sha256sums=(/,/)/p' packaging/arch/PKGBUILD
   grep -F "sha256sums=('${TARBALL_HASH}')" packaging/arch/PKGBUILD
   (cd packaging/arch && makepkg --verifysource)   # must print "Passed"
   (cd packaging/arch && makepkg --printsrcinfo > .SRCINFO)  # the .SRCINFO the AUR expects
   ```

   Commit the updated PKGBUILD (and the regenerated `.SRCINFO`) and push —
   to the repository and to the AUR. This is the step that makes the Arch
   package buildable from the tag; the tag itself never changes.

5. Run the full source build and record its result as a genuine full success.
   The workflow's Arch job runs only the metadata/consistency validation by
   default; the full build (which also verifies the checksum) is opt-in through
   the "Full source build" checkbox on a manual dispatch. **Do not trigger a
   manual `arch_full_build` dispatch from the tag ref**: the checked-out ref
   contains the stale tag-time PKGBUILD, and the build would fail the
   `sha256sums` check by design. Use the branch with the post-tag checksum
   committed (step 4), or run `makepkg --syncdeps --noconfirm` locally.

Do not move the existing tag to fix a release, and do not bypass the checksum
with `--skipchecksums`/`--skipinteg` or by dropping `sha256sums`. If a release
tag is broken (e.g. its sources are pre-fix), cut a new version and tag from a
fixed main (see [v0.3.0 release](#v030-release-checksum-only-vs-full-arch-success)).

`makepkg` builds from the tag tarball, not from the checkout, so the PKGBUILD
source URL is derived from `pkgver`. The release workflow's Arch job validates
the PKGBUILD syntax and the `pkgver`/trigger consistency on every trigger, and
— on a tag push — that the source URL resolves to a tarball whose project
version matches the PKGBUILD; the tarball checksum is verified in the full
source build (opt-in on manual dispatch), never masked by skipping the check.

## Validation and diagnostics

Useful commands by purpose:

| Purpose | Command |
| ------- | ------- |
| PKGBUILD syntax + metadata | `makepkg --printsrcinfo` (inside `packaging/arch`) |
| Tarball checksum, without building | `makepkg --verifysource` |
| Full source build (downloads, verifies, compiles everything) | `makepkg --syncdeps --noconfirm` |
| `.deb` + `.tar.xz` | `cmake --workflow --preset package` |
| RPM | `cmake --workflow --preset package-rpm` (Fedora-family container) |
| AppImage | `bash packaging/linux/build-appimage.sh` |
| Windows ZIP / macOS ZIP | `cmake --preset release` + `cpack --preset release` |
| ZIP/DMG/pkg contents | `unzip -l <zip>`; `hdiutil imageinfo <dmg>` (macOS) |
| Installed package contents | `dpkg-deb -c <.deb>`, `rpm -qlp <.rpm>`, `tar -tf <.tar.xz>`, `tar -tvf <pkg.tar.zst>` |

Quick checks after any packaging change:

- Pre-tag, per the procedure: `makepkg --printsrcinfo` prints valid metadata
  whose `source` points at the expected `v<pkgver>.tar.gz`. There is no
  checksum check yet — no tarball exists for the new tag, so `sha256sums`
  carries the provisional previous-tag hash (recorded so step 4 has a value to
  replace; it is wrong on purpose and never a reason to skip or bypass) until
  step 4.
- Post-tag, per the procedure: `makepkg --verifysource` downloads the tag's
  tarball and must print `Passed` (the checksum committed in step 4). This
  check only exists once the tag is on the remote.
- Arch source integrity: `packaging/arch/check-sources.sh` (as the build user)
  runs `makepkg --verifysource` on the tag tarball (it requires the corrected
  checksum committed in step 4), requires the NuGet fix in the tag's
  `cmake/Engines.cmake`, and fails only if the obsolete v0.3.0 backport patch
  reappears — no file, no PKGBUILD reference. Local source files of other
  vintages (e.g. a future pinned backport) are legitimate sources: the script
  stages them next to the PKGBUILD and lets `makepkg --verifysource` check
  them against the recorded `sha256sums`. The release workflow runs it before
  a requested manual full build from the post-tag branch, not on a tag push.
- The release workflow's Arch job runs the metadata/`pkgver`-consistency
  validation (and, on a tag push, the tarball-version check) on every
  trigger. It runs `makepkg --verifysource` only on a manual full build
  from the post-tag branch; the full build
  (`makepkg --syncdeps --noconfirm`) is opt-in through the "Full source build"
  checkbox on a manual dispatch.
- Package contents: engines in the right directory (`libexec/ducktor` on Linux,
  next to the app on Windows/macOS), `licenses/` present, no AI models.

## Troubleshooting

### The PKGBUILD checksum fails (`v<version>.tar.gz ... FAILED`) — verified cause

Symptom (from `makepkg`):

```text
==> Validating source files with sha256sums...
    v0.3.0.tar.gz ... FAILED
==> ERROR: One or more files did not pass the validity check!
```

Verified cause in the v0.3.0 release: the `pkgver` bump (commit `b446eae`)
left the previous release's checksum (`422b946...`, the v0.2.1 tarball) in
place — the dedicated checksum commit that v0.2.1 used (`291458f`) was never
made for v0.3.0. The tag's own tarball hashes differently.

How to diagnose:

```bash
cd packaging/arch
makepkg --verifysource   # prints the hash mismatch without building
sha256sum <downloaded tarball>   # from:
# https://github.com/Celtic-Bytes/Ducktor/archive/refs/tags/v<pkgver>.tar.gz
```

Fix:

1. Download the tarball of the exact tag the PKGBUILD is built from.
2. Compute its SHA-256 and put it in `sha256sums` (replacing the stale value).
3. Re-run `makepkg --verifysource` — it must print `Passed`.
4. Keep the check enabled: never work around a mismatch with
   `--skipchecksums` / `--skipinteg` or by dropping `sha256sums`; a mismatch
   means the tarball does not match what the release recorded, and is worth
   investigating (stale checksum after a bump, or the tag contents changed).

Known structural constraint: the tarball contains the PKGBUILD itself, so a
tag's own PKGBUILD can never carry the correct checksum for that same tag —
the tarball hash is only knowable once the tag exists, which is why the
release procedure recalculates the checksum after tagging. The workflow's Arch
job therefore only checks syntax, metadata and `pkgver` consistency (and, on a
tag push, the tarball-version check) in its fast validation step; the tarball
checksum is verified in the full source build (opt-in on manual dispatch).
Both triggers run the same validation, but a genuinely stale checksum is caught
whenever the full build runs — never masked by skipping the check.

### v0.3.0 release: checksum-only vs full Arch success

Fixing the PKGBUILD checksum (step 4 above) is not the same as a successful
full Arch build, and the report for that fix must not imply otherwise.

Verified facts for the v0.3.0 tag (`b446eae`, Nov 26 14:12, before the two
build fixes below):

- `makepkg --verifysource` on the corrected PKGBUILD passes: the tarball
  `v0.3.0.tar.gz` hashes to
  `60b41c8e8e8f6e623674c858802ab0ebd920d6516dd7829bf76aae5e3c5e9636`. This
  verifies the checksum path only.
- Building the tag's sources with the tag's own `cmake/Engines.cmake` fails in
  the piper configure step. Reproduced in a container:
  `ducktor_libpiper` clones piper v1.8.0 and libpiper tries to download ONNX
  Runtime `1.22.0` from the removed `microsoft/onnxruntime` GitHub release,
  ends on a 404 page, and `find_library(onnxruntime ...)` fails at piper's
  `libpiper/CMakeLists.txt:132`. **The full tagged build fails; the checksum
  fix does not change this.**

The two fixes that landed after the tag are both in current `main`:

- `b1d24eb` — ONNX Runtime is fetched from the pinned NuGet package
  (`Microsoft.ML.OnnxRuntime` `1.22.0`, SHA-256 verified) instead of the
  removed GitHub release; the layout libpiper expects is prepared at build
  time and passed with `CMAKE_DISABLE_FIND_PACKAGE_onnxruntime=TRUE`.
- `a3958ed` — piper's nested espeak-ng is handed the `__has_include_next`
  fallback MSVC 14.52 needs, applied as a build-time patch (see "Windows:
  Piper's nested espeak-ng build fails on MSVC 14.52" below).

Verified on `main` (in an Arch container, the speak-engine path only):
`ducktor_onnxruntime` (NuGet, hash-checked) and `ducktor_libpiper` (piper
v1.8.0 with the patch) both build and install.

Full package build of the **v0.3.0 tag with the backport patch** (see
[v0.3.0 backport](#v030-backport-arch-package)) was then run in the same
container: all engines, the full package and the contents checks pass — see
the verified-result block there. The full build on `main` itself and the
Windows/macOS full builds are still **not** verified.

Safe resolution (do not do either of the forbidden things: move the v0.3.0 tag
or bypass the checksum) — the procedure to follow for a release broken in the
sources:

1. Cut a **new version and tag** from a main that carries both fixes — the
   tag's tarball then carries fixed sources. This is verified only once the
   full build actually runs; it is not implied by the fixes alone.
2. Recalculate `sha256sums` from that new tag's tarball and commit the result
   after tagging (the structural constraint above); push to the AUR.
3. Only then run the full Arch build (`makepkg --syncdeps --noconfirm`), and
   record its result as a genuine full success.

The v0.3.0 tag may be left as is: it is honest about being a metadata/release
tag, and the release procedure for the next tag does not require moving it.

A fourth option — used for this repair and now **historical** — was to package
the v0.3.0 tag with the local backport patch described in
[v0.3.0 backport](#v030-backport-arch-package): the tag is not moved, the
`sha256sums` check is not bypassed and the patch is a pinned, hash-verified
source like the tarball. The upstream fix `b1d24eb` is backported exactly as
reviewed; the MSVC espeak-ng fix `a3958ed` is deliberately **not** backported
(Windows-only, touches files the tag does not carry, and not needed by the
Linux package). The backport was guarded on `pkgver == "0.3.0"` and was removed together
with the release that followed it, from a tree that carries the fix upstream.

### Windows: Piper's nested espeak-ng build fails on MSVC 14.52

Symptom: errors `C7780` / `C1017` while the nested espeak-ng project builds
as part of piper v1.8.0.

Root cause: piper v1.8.0 pins an espeak-ng commit whose compat shims probe the
C++23 `__has_include_next` operator without a `defined()` guard. MSVC does not
implement it: MSVC 14.44 (Visual Studio 2022) treated the undefined
function-like macro as 0, but MSVC 14.52 (Visual Studio 2026) rejects the
construct outright. Upstream espeak-ng added a fallback later, but piper v1.8.0
still pins the older commit and no piper release carries the fix.

Workaround (kept in the tree):

- [cmake/piper-espeak-ng-compat.patch](cmake/piper-espeak-ng-compat.patch) is
  applied to the piper checkout at build time by
  [cmake/ApplyPiperEspeakNgCompat.cmake](cmake/ApplyPiperEspeakNgCompat.cmake)
  (it fails loudly if the pin moves and the patch no longer applies).
- The patch adds `CMAKE_PROJECT_INCLUDE_BEFORE` with
  [cmake/EspeakNgCompat.cmake](cmake/EspeakNgCompat.cmake), which force-includes
  [cmake/espeak-ng-msvc-compat.h](cmake/espeak-ng-msvc-compat.h) with `/FI` on
  MSVC only — a function-like `__has_include_next(x) 0` fallback (MSVC's `/D`
  switch cannot define function-like macros, which is why `/FI` is used).

Do not bump piper past the pin without re-checking this patch, and do not
remove the `/FI` include — it is what makes the pinned espeak-ng build.

### ONNX Runtime download fails (every package build fails while configuring piper)

Symptom: libpiper's own download of ONNX Runtime ends with a 404 page (its
GitHub-release download has no failure check), so the piper configure step
fails before any compilation.

Cause: Microsoft removed the `microsoft/onnxruntime` repository and its
release assets. Upstream piper v1.8.0 still downloads ONNX Runtime from that
location.

Fix (in the tree): [cmake/Engines.cmake](cmake/Engines.cmake) downloads the
official NuGet package `Microsoft.ML.OnnxRuntime` `1.22.0` with a pinned
SHA-256 (`URL_HASH SHA256=d571e63a...`), prepares the layout libpiper expects,
and passes `ONNXRUNTIME_DIR`/`ONNXRUNTIME_LIB` with
`CMAKE_DISABLE_FIND_PACKAGE_onnxruntime=TRUE` so libpiper never downloads it
itself. Keep the NuGet pin and its hash in sync with the version; do not
point libpiper back at the GitHub release.

### `.deb`/`RPM` dependencies miss runtime Qt plugins and QML modules

Symptom: `dpkg-shlibdeps` (`.deb`) / the RPM generator only list libraries;
the QML modules and plugins that Ducktor loads at runtime (SVG icon engine,
Qt Multimedia, Qt Quick, Qt Qml/WorkerScript, platform plugins) are missing.

Fix: keep the dependency derivation in
[cmake/DebianQtDepends.cmake](cmake/DebianQtDepends.cmake) (`dpkg -S`) and
[cmake/FedoraQtDepends.cmake](cmake/FedoraQtDepends.cmake) (`rpm -q --whatprovides qt6qml(...)`)
in sync with the runtime files Ducktor loads — do not hand-edit the generated
dependencies or rely on rpmbuild alone. QML modules and plugins must be
installed on the build machine for the derivation to see them.

### eSpeak NG fails with very long build paths

Symptom: the piper/espeak-ng build fails while compiling phoneme data.

Fix: keep the repository in a reasonably short path — long paths are
truncated by eSpeak NG. This affects local and CI builds equally; do not work
around it by disabling espeak-ng.

### Flatpak build needs network

[packaging/flatpak/com.celticbytes.Ducktor.yml](packaging/flatpak/com.celticbytes.Ducktor.yml)
builds from the repo tree (`type: dir`) and allows `--share=network` during the
build because the engines download their sources. That is fine for local
builds; publishing on Flathub would require declaring every source in the
manifest instead. Do not "fix" the manifest by adding more network access.

## Keeping this document updated

- Any packaging-affecting change should update this file and the matching
  section of [README.md](README.md) in the same change: install layout,
  dependency derivation, pins, or the release procedure.
- Any new dependency: pin it, verify it (SHA-256/git tag), copy its license
  text during the build, and list it in
  [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
- When a packaging step fails, record the failure mode here (symptom, cause,
  fix, with the exact commands) — as archived above for the v0.3.0 Arch
  checksum and the two Windows build failures — after the cause is verified,
  not before.
- The Arch PKGBUILD is verified by running `makepkg --verifysource` (hash
  check) on the post-tag state and the release workflow's Arch job; re-run
  them when bumping `pkgver` or after any release-procedure change.
- Pre-tag on a clean checkout: confirm `makepkg --printsrcinfo` and the
  workflow's Arch job pass for the version being tagged (no tarball exists
  yet). Post-tag: confirm `makepkg --verifysource` passes, and only then
  record the release as checked.
