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
| [Hunspell](https://github.com/hunspell/hunspell) `v1.7.6`       | Spell-check engine               | [LGPL-2.1-or-later](https://github.com/hunspell/hunspell/blob/v1.7.6/COPYING.LESSER) (also offered under MPL-1.1 and GPL-2.0-or-later) |
| [Hunspell dictionaries](https://github.com/LibreOffice/dictionaries) | Bundled spell dictionaries   | One per language, see below; texts in [resources/spell/licenses](resources/spell/licenses)                                    |
| [flag-icons](https://github.com/lipis/flag-icons) `v7.6.0`      | Flags in the language menu       | [MIT](resources/flags/LICENSE)                                                                                                 |

The flag files in [resources/flags](resources/flags) are unmodified copies of `flags/4x3/{gb,es,fr,it,de}.svg` from flag-icons [`v7.6.0`](https://github.com/lipis/flag-icons/tree/v7.6.0/flags/4x3) (commit `7264def86ef4772ab40af4d6fcc78890e301ca6b`), Copyright (c) 2013 Panayiotis Lipiridis. English uses the United Kingdom flag.

The spell dictionaries in [resources/spell](resources/spell) are unmodified `.aff`/`.dic` copies from [LibreOffice/dictionaries](https://github.com/LibreOffice/dictionaries) (commit `c0865a85eb67dc377f2cf8e8fe2c4fa9e53f0dcb`), each under its own license:

| Dictionary    | Language | License                                                                                                       |
| ------------- | -------- | ------------------------------------------------------------------------------------------------------------- |
| `es_ES`       | Spanish  | [GPL-3.0-or-later](https://www.gnu.org/licenses/gpl-3.0.en.html), [LGPL-3.0-or-later](https://www.gnu.org/licenses/lgpl-3.0.en.html) or [MPL-1.1-or-later](https://www.mozilla.org/en-US/MPL/1.1/) (the user chooses) |
| `en_US`       | English  | Derived from [SCOWL](http://wordlist.sourceforge.net) (permissive, BSD-style) and Ispell (BSD)                |
| `fr`          | French   | [MPL-2.0](https://www.mozilla.org/en-US/MPL/2.0/)                                                             |
| `de_DE_frami` | German   | [GPL-2.0-or-3.0](https://www.gnu.org/licenses/gpl-2.0.en.html) (based on the Ispell `igerman98` word list, BSD) |
| `it_IT`       | Italian  | [GPL-3.0](https://www.gnu.org/licenses/gpl-3.0.en.html) (LibreItalia)                                         |

Their license texts are kept next to the files in [resources/spell/licenses](resources/spell/licenses). The files are verified by these SHA-256 checksums:

```text
646bf3333ac69c23e9d794533ee5241d6f755c359e8fe10a648f87613743d594  de_DE_frami.aff
e746c882dd6f303c2c46e7452804b9201115a6942cfeb15f18f8edf774d2e24e  en_US.aff
e73a9bf8e1383f4986a5dc9e2fbed49371c0c61f511c626d15586bd433c1cad9  es_ES.aff
c176610cd5dc4846806a65ddd029f422d87978bf58f224aa44222662a16a2de5  fr.aff
951afaa19272f13555b8823e8bcf9ccf78f8fe1a07835bdfb912ab3e4d537c2b  it_IT.aff
4ca3c958b0e5545910999bc246f668840bf8ede3df8e5e6790d05edd5a586c38  de_DE_frami.dic
f0b1a234bd178bdd01875b2a392a9647f888b8fe879f79c52aae62c2759b3647  en_US.dic
6975dddec3d5d2c676069537bc67b4b5f786c65c5d4cf6703a82acf779ac9ec1  es_ES.dic
b78a868e31dd6e373b6c3217969afb898a9acde828a5e7ef97308da42218c88c  fr.dic
bae1e3501dcd2a923669592493b3fde6c02aae7c7aab83bf5e5b49077e73dd64  it_IT.dic
```

## Binary packages

Every package has a `licenses` folder (in `/usr/share/doc/ducktor` on Linux) with the license texts of the components above: `llama.cpp-LICENSE`, `whisper.cpp-LICENSE`, `piper-COPYING`, `espeak-ng-COPYING`, `onnxruntime-LICENSE`, `onnxruntime-ThirdPartyNotices.txt`, `hunspell-LICENSE` and `flag-icons-LICENSE`. They are copied from each component's own source during the build. The spell dictionaries keep their own license texts next to the helper, in `spell/licenses/`.

The AppImage, the Windows ZIP and the macOS ZIP also contain a copy of the Qt libraries, linked dynamically so they can be replaced. Qt is distributed under the LGPL-3.0 ([LGPL-3.0-only.txt](packaging/licenses/LGPL-3.0-only.txt), from Qt `v6.10.0`), and its source code is available from [download.qt.io](https://download.qt.io/official_releases/qt/). On Windows and macOS, Qt Multimedia uses FFmpeg, distributed under the LGPL-2.1 or later ([LGPL-2.1-or-later.txt](packaging/licenses/LGPL-2.1-or-later.txt), from FFmpeg `n7.1`); its source code is available from [ffmpeg.org](https://ffmpeg.org/download.html). The Windows ZIP also includes the redistributable Microsoft Visual C++ runtime. The `.deb`, RPM and Flatpak packages use the Qt provided by the system or by the KDE runtime instead.

The tools used only to build packages ([linuxdeploy](https://github.com/linuxdeploy/linuxdeploy), [linuxdeploy-plugin-qt](https://github.com/linuxdeploy/linuxdeploy-plugin-qt), `windeployqt`, `macdeployqt`) and to generate the icons ([resvg-js](https://github.com/yisibl/resvg-js), MPL-2.0) are not included in them. The Ducktor logo and icons are original work licensed with Ducktor under the GPL-3.0-or-later.

## AI models

Ducktor does not include AI models in its source code. Each model is downloaded or imported by the user and keeps its own license, which may differ from Ducktor's. The license of every model is listed in [resources/models.json](resources/models.json) and shown in _Settings_ before downloading it.

Some Piper voices were trained on recordings licensed for non-commercial use only, or were fine-tuned from voices with restrictive or unreviewed terms; their entry says so (for example, "non-commercial" or "base: lessac"). Check that license before using the generated audio outside personal use.
