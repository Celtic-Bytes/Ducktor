# Ducktor

Offline desktop translator: text or voice input, local translation and read-aloud output. Linux is the primary platform; Windows and macOS builds are also supported.

Status: first functional version of the code, **not yet built or validated on real machines**. Model figures come from their authors and have not yet been measured on 8 GB computers.

## Architecture

- Interface: Qt 6.9+ with Qt Quick and C++ ([Qt documentation](https://doc.qt.io/qt-6/)).
- Each inference engine is a separate helper program that communicates through local pipes, with no server or network:
  - `ducktor-translate`: [llama.cpp](https://github.com/ggml-org/llama.cpp) `v0.7.0` with GGUF models.
  - `ducktor-transcribe`: [whisper.cpp](https://github.com/ggml-org/whisper.cpp) `v1.9.5`.
  - `ducktor-speak`: [libpiper](https://github.com/OHF-Voice/piper1-gpl/tree/main/libpiper) `v1.8.0`.
- If a model runs out of memory or crashes, its process is closed and the interface keeps responding.
- Models are downloaded from Settings, with resumable downloads and SHA-256 verification, or imported from a local file.

## Build requirements

- CMake 3.24 or later and Ninja.
- A C++17 compiler: GCC or Clang on Linux, MSVC 2022 on Windows, Xcode on macOS.
- Qt 6.9 or later with the Quick, Quick Controls, Multimedia, Network, Svg and Linguist Tools modules.
- Git and an internet connection during the first configuration, to fetch the engines.

On Debian or PikaOS, the distribution's Qt packages may be older than 6.9; in that case, install Qt with the [online installer](https://doc.qt.io/qt-6/get-and-install-qt.html).

## Build and test

```bash
cmake --preset dev -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build --preset dev
ctest --preset dev
./build/dev/bin/ducktor
```

On Windows, run the same commands from the _x64 Native Tools Command Prompt for VS 2022_. The `ui-only` preset builds the interface and tests without fetching or building the engines.

To make a portable build, place an empty `portable.txt` file next to the executable: settings and models will be stored in a `data` folder beside it.

## Licenses

The proposed license for Ducktor is GPL-3.0-or-later, compatible with libpiper (GPL-3.0); it is pending confirmation. Each model has its own license, shown in Settings before downloading it.
