# Ducktor

Offline desktop translator: text or voice input, local translation and read-aloud output. Linux is the primary platform; Windows and macOS are planned targets.

Status: first functional version of the code, **not yet built or validated on real machines**. Model figures come from their authors and have not yet been measured on 8 GB computers.

## Technologies in brief

| Piece                                                                                                                                                        | What it is                              | Why Ducktor uses it                                               |
| ------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------- | ----------------------------------------------------------------- |
| [C++](https://isocpp.org/)                                                                                                                                   | Compiled programming language           | Runs the AI engines efficiently on modest CPUs                    |
| [Qt](https://doc.qt.io/qt-6/)                                                                                                                                | Cross-platform toolkit for desktop apps | Window, buttons, audio and downloads on Linux, Windows and macOS  |
| [QML / Qt Quick](https://doc.qt.io/qt-6/qtquick-index.html)                                                                                                  | Qt's language for describing interfaces | The screens in the [qml](qml) folder                              |
| [CMake](https://cmake.org/cmake/help/latest/)                                                                                                                | Build system                            | Turns the source code into executables                            |
| [Ninja](https://ninja-build.org/)                                                                                                                            | Fast build runner used by CMake         | Required by the presets in [CMakePresets.json](CMakePresets.json) |
| [llama.cpp](https://github.com/ggml-org/llama.cpp), [whisper.cpp](https://github.com/ggml-org/whisper.cpp), [Piper](https://github.com/OHF-Voice/piper1-gpl) | Local AI engines                        | Translation, speech recognition and speech synthesis              |

## Architecture

- Interface: Qt 6.9+ with Qt Quick and C++ ([Qt documentation](https://doc.qt.io/qt-6/)).
- Each inference engine is a separate helper program that communicates through local pipes, with no server or network:
  - `ducktor-translate`: [llama.cpp](https://github.com/ggml-org/llama.cpp) `v0.7.0` with GGUF models.
  - `ducktor-transcribe`: [whisper.cpp](https://github.com/ggml-org/whisper.cpp) `v1.9.5`.
  - `ducktor-speak`: [libpiper](https://github.com/OHF-Voice/piper1-gpl/tree/main/libpiper) `v1.8.0`.
- If a model runs out of memory or crashes, its process is closed and the interface keeps responding.
- Models are downloaded from Settings, with resumable downloads and SHA-256 verification, or imported from a local file.

## What to install

You need five things: a C++ compiler, CMake, Ninja, Git and Qt 6.9 or later. Qt must include the **Qt Multimedia** module, which is not selected by default in the Qt installer.

### Windows 10/11

1. Install Git, CMake, Ninja and the Microsoft C++ compiler from PowerShell:

   ```powershell
   winget install Git.Git Kitware.CMake Ninja-build.Ninja
   winget install Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
   ```

   The second command installs [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/), which are free; the full Visual Studio IDE is not needed.

2. Install Qt with the [Qt Online Installer](https://doc.qt.io/qt-6/get-and-install-qt.html) (it asks for a free Qt account). In the component list, open the latest Qt 6 version (6.9 or later) and select:
   - **MSVC 2022 64-bit**
   - **Additional Libraries → Qt Multimedia**

   Qt is installed by default in a folder such as `C:\Qt\6.9.3\msvc2022_64`. Write down the exact path: you will need it.

3. Restart the computer so the new programs are found.

Build commands on Windows must run in the **x64 Native Tools Command Prompt for VS 2022** (search for it in the Start menu), which prepares the compiler.

### Linux (Debian, Ubuntu, PikaOS)

1. Install the build tools and the system libraries Qt needs:

   ```bash
   sudo apt install build-essential cmake ninja-build git libgl1-mesa-dev libxkbcommon-dev libxcb-cursor0 libpulse-dev
   ```

   CMake must be 3.24 or later (`cmake --version`).

2. Install Qt with the [Qt Online Installer](https://doc.qt.io/qt-6/get-and-install-qt.html), selecting **Desktop gcc 64-bit** and **Additional Libraries → Qt Multimedia** for Qt 6.9 or later. The default path looks like `~/Qt/6.9.3/gcc_64`.

   Your distribution's Qt packages also work if `apt policy qt6-base-dev` shows version 6.9 or later (PikaOS ships a recent enough Qt). In that case, install these packages instead and skip `CMAKE_PREFIX_PATH` below:

   ```bash
   sudo apt install qt6-base-dev qt6-declarative-dev qt6-multimedia-dev qt6-svg-dev qt6-tools-dev \
       qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts \
       qml6-module-qtquick-dialogs qml6-module-qtquick-window qml6-module-qtquick-templates \
       qml6-module-qtqml-workerscript
   ```

   The `qml6-module-*` packages are needed to start the app; without them Ducktor exits with a message such as `module "QtQuick.Controls" is not installed`.

## Run in development mode

### From the terminal

Replace the Qt path with yours:

```bash
cmake --preset dev -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.3/gcc_64
cmake --build --preset dev
ctest --preset dev
./build/dev/bin/ducktor
```

On Windows, use the Windows path (`-DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64`) and start the app with `build\dev\bin\ducktor.exe`. Before starting it, add Qt to the `PATH` of that terminal, otherwise Windows cannot find the Qt DLLs:

```bat
set PATH=C:\Qt\6.9.3\msvc2022_64\bin;%PATH%
```

What to expect:

- The first configuration downloads llama.cpp, whisper.cpp and Piper, and the first build compiles them. It can take a long time; later builds are much faster.
- With your distribution's Qt, configuration may print many `The qml plugin '...' ... will not be linked` warnings. They are harmless: those plugins are loaded from the system when Ducktor starts.
- Ducktor starts with no AI models. Open **Settings** and download at least one translation model; for voice input and output, also download a speech recognition model and a voice. The recommended set is about 1.5 GB.
- To work only on the interface, use the `ui-only` preset instead of `dev`: it skips the engines, so it builds quickly, but translation and voice will report that a component is missing.
- To see the engines' diagnostic output, set the environment variable `DUCKTOR_DEBUG_ENGINES=1` before starting Ducktor. Their output can include fragments of the processed text, so use it only for debugging.

### From Visual Studio Code

1. Open the repository folder in VS Code and install the recommended extensions when prompted. They are listed in [.vscode/extensions.json](.vscode/extensions.json):
   - [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools): configure, build, test and run from the status bar.
   - [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools): code completion and debugging.
   - [Qt Extension Pack](https://marketplace.visualstudio.com/items?itemName=theqtcompany.qt): QML highlighting and Qt tools.
2. So you do not have to type the Qt path every time, create a file named `CMakeUserPresets.json` in the repository root. It is personal and ignored by Git:

   ```json
   {
     "version": 6,
     "configurePresets": [
       {
         "name": "my-dev",
         "inherits": "dev",
         "cacheVariables": { "CMAKE_PREFIX_PATH": "C:/Qt/6.9.3/msvc2022_64" }
       }
     ],
     "buildPresets": [{ "name": "my-dev", "configurePreset": "my-dev" }],
     "testPresets": [{ "name": "my-dev", "configurePreset": "my-dev" }]
   }
   ```

   With your distribution's Qt on Debian, Ubuntu or PikaOS, use `"Qt6_DIR": "/usr/lib/x86_64-linux-gnu/cmake/Qt6"` instead of `CMAKE_PREFIX_PATH`. Without one of them, VS Code shows the error `Could not find a package configuration file provided by "Qt6"`.

3. In the CMake panel, select the `my-dev` configure preset, then use **Build**, **Run Tests** and **Launch** (or **Debug**) with the `ducktor` target.

On Windows, start VS Code from the _x64 Native Tools Command Prompt for VS 2022_ (run `code .` in the repository folder) so CMake can find the compiler.

## Packages

**Package generation is not implemented yet.** There are currently no commands to create the planned distribution formats, and the build folder is not self-contained: `build/<preset>/bin` contains Ducktor and its engines, but not Qt or the AI models.

Planned formats:

| System  | Format                                  |
| ------- | --------------------------------------- |
| Linux   | `.deb`, Flatpak and AppImage (portable) |
| Windows | Portable ZIP, no installer              |
| macOS   | `.app` inside a ZIP                     |

The `release` preset already produces optimized binaries that run on other CPUs (`cmake --preset release`), as the first step towards those packages. Packaging commands and output folders will be documented here once they exist.

To make a portable build, place an empty `portable.txt` file next to the executable: settings and models will be stored in a `data` folder beside it.

## Privacy

Ducktor has no accounts, telemetry or history, and your text and voice never leave your computer. It only connects to the internet when you download a model. Details: [PRIVACY.md](PRIVACY.md).

## License

Copyright (C) 2026 celticbytes.

Ducktor is free software: you can redistribute it and/or modify it under the terms of the [GNU General Public License](LICENSE), version 3 or (at your option) any later version. It is distributed in the hope that it will be useful, but **without any warranty**; machine translations and transcriptions can contain errors, so check them before relying on them for important matters.

Third-party components and their licenses are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Each AI model has its own license, shown in Settings before downloading it.
