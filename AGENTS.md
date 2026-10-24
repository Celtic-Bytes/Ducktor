# Ducktor agent guide

Offline desktop translator (Qt 6.9 + C++17). Build steps and architecture overview: [README.md](README.md). Repository docs, code, comments and UI source strings are in English; chat with the maintainer is in Spanish.

## Build and test

- Presets in [CMakePresets.json](CMakePresets.json); Ninja is required. `ui-only` skips the inference engines and is the fastest way to build the app and run `ctest`.
- Engines are separate CMake projects added via `ExternalProject` in [cmake/Engines.cmake](cmake/Engines.cmake): llama.cpp and whisper.cpp both vendor ggml, so never `add_subdirectory` them into the same project.
- Pinned versions: llama.cpp `v0.7.0`, whisper.cpp `v1.9.5`, piper1-gpl `v1.8.0`. Before using a C API, check the header at that tag, not `master`.
- If the machine has no compiler/Qt, say so plainly; never claim something compiled or tests passed without running them.

## Packaging

- Install layout lives in [cmake/Install.cmake](cmake/Install.cmake) and must match the search paths in [Paths.cpp](src/core/Paths.cpp) (Linux engines in `libexec/ducktor`, Windows next to `ducktor.exe`, macOS in `Ducktor.app/Contents/MacOS`).
- CPack ([cmake/Packaging.cmake](cmake/Packaging.cmake)) makes the `.deb` and the Windows/macOS ZIPs; AppImage and Flatpak live in [packaging/](packaging). Output always goes to `dist/`.
- When a dependency is added, copy its license text into `${DUCKTOR_LICENSE_OUTPUT_DIR}` from its own sources during the build, and list it in THIRD_PARTY_NOTICES.md.
- Download tools only from pinned versions and verify their SHA-256, as in [build-appimage.sh](packaging/linux/build-appimage.sh).

## Architecture rules

- The GUI never links inference libraries. Each engine is a helper process (`engines/<name>/main.cpp`) spawned by [EngineProcess](src/engines/EngineProcess.cpp), so a crash or OOM only kills that helper.
- Helper protocol: header line `FIELD ... <payloadBytes>\n` followed by the raw payload. Writer: [engines/common/protocol.hpp](engines/common/protocol.hpp); reader: [FrameParser](src/engines/FrameParser.cpp). Change both sides together and keep fields free of spaces; user text always goes in the payload, never in arguments.
- [AppController](src/core/AppController.cpp) runs one heavy task at a time (`state` string) and ignores frames whose request id is stale. Cancelling kills the helper.
- QML talks to C++ only through the singletons `App`, `Models` and `Settings` registered in [src/main.cpp](src/main.cpp) (`import Ducktor.Core`).

## Models and privacy

- [resources/models.json](resources/models.json) is the only source of downloadable models: `https://huggingface.co` URLs pinned to a commit revision, plus exact `size` and `sha256`. Verify these against the Hugging Face API instead of guessing. The parser in [ModelCatalog](src/models/ModelCatalog.cpp) rejects anything else.
- Only list models whose engine Ducktor can run; the parser ignores other engines. Track candidate engines in the README roadmap, not in the catalog.
- Piper voices need `gender` (`female`/`male`) or `speakers` (`{"female": id, "male": id}`). libpiper `v1.8.0` only handles `espeak`, `text` and `pinyin` (with extra dictionaries) phonemes: check `phoneme_type` in the voice's `.onnx.json` before adding it.
- Check each model's and voice's license separately (weights, dataset, fine-tune base). Restrictive or unreviewed licenses must not be bundled or marked recommended.
- No telemetry, history or logging of user text/audio. Helper stderr is discarded unless `DUCKTOR_DEBUG_ENGINES` is set.
- Ducktor is GPL-3.0-or-later. Update [PRIVACY.md](PRIVACY.md) when data handling or network use changes, and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) when dependencies change. Never add an EULA or terms that restrict GPL rights.

## Limits

- Input limits live in [AppSettings](src/core/AppSettings.h): users may lower them but never exceed the hard maxima (5000 chars, 120 s) until measured on 8 GB CPU-only machines.
- Enforce limits before data is inserted or sent (see [LimitedTextArea.qml](qml/LimitedTextArea.qml)) and again in the helper. Never truncate silently; report it to the user.

## Translations

- Wrap UI strings in `qsTr()`/`tr()` and add the entry to every file in [i18n/](i18n/) (`es`, `fr`, `it`, `de`; English is the source). The context is the QML file basename or the C++ class name. Catalog descriptions in `models.json` use the same language keys and fall back to `en`.
- The interface language is the `Settings.uiLanguage` setting (default English) and switches live through `QQmlEngine::retranslate()` in [src/main.cpp](src/main.cpp).
