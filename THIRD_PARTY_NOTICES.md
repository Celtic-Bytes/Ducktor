# Third-party notices

Ducktor is licensed under the [GNU General Public License v3.0 or later](LICENSE). It builds on the following open-source components, each under its own license. Binary packages must include these license texts.

| Component                                                       | Used for                         | License                                                                                                                        |
| --------------------------------------------------------------- | -------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| [Qt 6](https://www.qt.io/)                                      | Interface, audio, networking     | [LGPL-3.0](https://doc.qt.io/qt-6/lgpl.html) (some modules GPL-3.0); see [Qt licensing](https://doc.qt.io/qt-6/licensing.html) |
| [llama.cpp](https://github.com/ggml-org/llama.cpp) and ggml     | Translation engine               | [MIT](https://github.com/ggml-org/llama.cpp/blob/master/LICENSE)                                                               |
| [whisper.cpp](https://github.com/ggml-org/whisper.cpp) and ggml | Speech recognition engine        | [MIT](https://github.com/ggml-org/whisper.cpp/blob/master/LICENSE)                                                             |
| [Piper (libpiper)](https://github.com/OHF-Voice/piper1-gpl)     | Speech synthesis engine          | [GPL-3.0](https://github.com/OHF-Voice/piper1-gpl/blob/main/COPYING)                                                           |
| [eSpeak NG](https://github.com/espeak-ng/espeak-ng)             | Pronunciation data for Piper     | [GPL-3.0-or-later](https://github.com/espeak-ng/espeak-ng/blob/master/COPYING)                                                 |
| [ONNX Runtime](https://github.com/microsoft/onnxruntime)        | Neural network runtime for Piper | [MIT](https://github.com/microsoft/onnxruntime/blob/main/LICENSE)                                                              |
| [flag-icons](https://github.com/lipis/flag-icons) `v7.6.0`      | Flags in the language menu       | [MIT](resources/flags/LICENSE)                                                                                                 |

The flag files in [resources/flags](resources/flags) are unmodified copies of `flags/4x3/{gb,es,fr,it,de}.svg` from flag-icons [`v7.6.0`](https://github.com/lipis/flag-icons/tree/v7.6.0/flags/4x3) (commit `7264def86ef4772ab40af4d6fcc78890e301ca6b`), Copyright (c) 2013 Panayiotis Lipiridis. English uses the United Kingdom flag.

## Binary packages

Every package has a `licenses` folder (in `/usr/share/doc/ducktor` on Linux) with the license texts of the components above: `llama.cpp-LICENSE`, `whisper.cpp-LICENSE`, `piper-COPYING`, `espeak-ng-COPYING`, `onnxruntime-LICENSE`, `onnxruntime-ThirdPartyNotices.txt` and `flag-icons-LICENSE`. They are copied from each component's own source during the build.

The AppImage, the Windows ZIP and the macOS ZIP also contain a copy of the Qt libraries, linked dynamically so they can be replaced. Qt is distributed under the LGPL-3.0 ([LGPL-3.0-only.txt](packaging/licenses/LGPL-3.0-only.txt), from Qt `v6.10.0`), and its source code is available from [download.qt.io](https://download.qt.io/official_releases/qt/). On Windows and macOS, Qt Multimedia uses FFmpeg, distributed under the LGPL-2.1 or later ([LGPL-2.1-or-later.txt](packaging/licenses/LGPL-2.1-or-later.txt), from FFmpeg `n7.1`); its source code is available from [ffmpeg.org](https://ffmpeg.org/download.html). The Windows ZIP also includes the redistributable Microsoft Visual C++ runtime. The `.deb` and Flatpak packages use the Qt provided by the system or by the KDE runtime instead.

The tools used only to build packages ([linuxdeploy](https://github.com/linuxdeploy/linuxdeploy), [linuxdeploy-plugin-qt](https://github.com/linuxdeploy/linuxdeploy-plugin-qt), `windeployqt`, `macdeployqt`) and to generate the icons ([resvg-js](https://github.com/yisibl/resvg-js), MPL-2.0) are not included in them. The Ducktor logo and icons are original work licensed with Ducktor under the GPL-3.0-or-later.

## AI models

Ducktor does not include AI models in its source code. Each model is downloaded or imported by the user and keeps its own license, which may differ from Ducktor's. The license of every model is listed in [resources/models.json](resources/models.json) and shown in _Settings_ before downloading it.

Some Piper voices were trained on recordings licensed for non-commercial use only, or were fine-tuned from voices with restrictive or unreviewed terms; their entry says so (for example, "non-commercial" or "base: lessac"). Check that license before using the generated audio outside personal use.
