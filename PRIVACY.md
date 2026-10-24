# Privacy policy

Ducktor is a free, open-source translator that works on your computer. It is designed so that your text and voice never leave it.

## What Ducktor does not do

- No accounts, sign-in or registration.
- No telemetry, analytics, crash reports, advertising or tracking of any kind.
- No sending of your text, recordings or translations to any server, including ours.
- No history: Ducktor does not save what you type, dictate or translate.

## How your content is processed

- **Text** is translated on this computer by a local helper program. It stays in memory while the window is open and is discarded when you clear it or close Ducktor.
- **Voice** is recorded only while you press _Dictate_, up to the limit shown on screen. The recording is kept in memory, transcribed locally and then discarded; it is never written to disk.
- **Speech output** is generated locally and played directly; no audio file is saved.
- **Clipboard**: Ducktor reads the clipboard only when you paste, and writes to it only when you press _Copy_.
- The local helper programs do not write logs. Developers can enable diagnostic output with the `DUCKTOR_DEBUG_ENGINES` environment variable; it goes to the terminal, never to a file, and may contain processed text.

## What Ducktor stores on your computer

- `settings.ini`: your limits, the interface language, the voice playback preference and the models you selected.
- `models/`: the AI models you downloaded or imported, plus unfinished downloads (`.part` files) so they can be resumed.

These files are stored in:

| System        | Folder                                                        |
| ------------- | ------------------------------------------------------------- |
| Windows       | `%LOCALAPPDATA%\celticbytes\Ducktor`                          |
| Linux         | `~/.local/share/celticbytes/Ducktor`                          |
| Linux Flatpak | `~/.var/app/com.celticbytes.Ducktor/data/celticbytes/Ducktor` |
| macOS         | `~/Library/Application Support/celticbytes/Ducktor`           |
| Portable copy | The `data` folder next to the executable                      |

The exact folder is shown in _Settings → General_. Deleting it removes all Ducktor data from your computer.

## When Ducktor uses the internet

Ducktor connects to the internet only when you press _Download_ for a model. It then requests the model files from [Hugging Face](https://huggingface.co), which may redirect to its content delivery network. As with any download, Hugging Face receives your IP address, the requested file and the program name ("Ducktor"); see the [Hugging Face privacy policy](https://huggingface.co/privacy). No text, audio or personal information is sent.

Links in _Settings_ (licenses, model pages) open in your web browser only when you click them.

Ducktor works fully offline once its models are installed. Models can also be installed without a connection with _Import file_.

## Microphone

Ducktor uses the microphone only after you press _Dictate_, and stops at the end of the recording. On systems that ask for permission, you can deny or revoke it at any time; text translation keeps working without it.

## Changes and contact

Any change to this policy is published in this file, together with the source code, so it can be reviewed. Ducktor is developed by [celticbytes](https://celticbytes.com).
