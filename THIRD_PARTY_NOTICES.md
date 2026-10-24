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

## AI models

Ducktor does not include AI models in its source code. Each model is downloaded or imported by the user and keeps its own license, which may differ from Ducktor's. The license of every model is listed in [resources/models.json](resources/models.json) and shown in _Settings_ before downloading it.

Some Piper voices were trained on recordings licensed for non-commercial use only, or were fine-tuned from voices with restrictive or unreviewed terms; their entry says so (for example, "non-commercial" or "base: lessac"). Check that license before using the generated audio outside personal use.
