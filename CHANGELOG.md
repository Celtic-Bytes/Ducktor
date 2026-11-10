# Changelog

All notable changes to Ducktor are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased](https://github.com/Celtic-Bytes/Ducktor/compare/v0.2.1...main)

## [0.2.1](https://github.com/Celtic-Bytes/Ducktor/compare/v0.2.0...v0.2.1) - 2026-11-10

### Changed

- The spell-check tab defaults to English instead of following the interface language, and the language chosen for a check is remembered across restarts

### Fixed

- The suggestion popup jumped to a neighboring misspelled word while the cursor travelled from the hovered (or clicked) word to its suggestions, so the wrong word's suggestions could be applied

## [0.2.0](https://github.com/Celtic-Bytes/Ducktor/compare/v0.1.1...v0.2.0) - 2026-11-10

### Added

- Spelling tab: offline spell-check built on Hunspell for Spanish, English, French, German and Italian, with the dictionaries bundled in the packages
- The error list shows each misspelled word with its context and up to eight suggestion chips; clicking a suggestion replaces the word and re-checks the text
- Copy and clear buttons for the text to check

## [0.1.1](https://github.com/Celtic-Bytes/Ducktor/compare/v0.1.0...v0.1.1) - 2026-11-06

### Added

- RPM package for the Fedora family (Fedora, openSUSE, ...): `ducktor-<version>-1.x86_64.rpm`
- `.tar.xz` package with the same FHS layout as the `.deb`, for distributions without a native package
- macOS DMG with a link to `/Applications` for drag-and-drop installation
- macOS `.pkg` installer that puts `Ducktor.app` in `/Applications`
- The Arch PKGBUILD in [packaging/arch](packaging/arch) is validated by the release workflow on every tag

### Changed

- The welcome window is centered and only re-appears when the app version changes
- The welcome message now includes a note about the expected translation quality

### Fixed

- A build path was leaking into the speech synthesis helper's RPATH in the Linux packages
- The speech synthesis libraries (libpiper, ONNX Runtime) were not shipped when building on systems where libpiper installs to `lib64`

## [0.1.0](https://github.com/Celtic-Bytes/Ducktor/releases/tag/v0.1.0) - 2026-11-04

### Added

- Initial alpha release
- Translation of typed or dictated text with local AI models (llama.cpp)
- Speech transcription with local models (whisper.cpp)
- Read-aloud of the translation with local voices (Piper)
- Interface in English, Spanish, French, Italian and German
- Model and voice downloads from Hugging Face, pinned by revision and checksum, with each license shown before downloading
- Welcome window on first launch
- Packages: `.deb` and AppImage for Linux, Flatpak, a portable ZIP for Windows, and a ZIP for macOS
