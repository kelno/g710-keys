# G710 keys

> Vibe-coded: the code, this README and the protocol notes were written by Claude (Anthropic's AI), and every step was tested on a real G710+.

Makes the Logitech G710+ G1-G6, M1-M3 and MR keys usable without Logitech's software, as keys that games and programs (AutoHotkey, PowerToys, OBS, Discord...) can bind. `g710.exe` uses about 2 MB of its own memory (1.8 MB private, 11 MB working set with shared Windows libraries, measured 2026-11-19), against 57 MB (100 MB) for Logitech Gaming Software.

## What the keys do

M1, M2 and M3 choose one of three sets for G1-G6; the chosen M key lights up, and the set is remembered across restarts.

| | Set M1 | Set M2 | Set M3 |
|---|---|---|---|
| G1-G6 | F13-F18 | F19-F24 | Ctrl+F13 to Ctrl+F18 |

MR sends Ctrl+F19 in every set. No real keyboard has F13-F24, so nothing else uses them.

To change them: tray icon (blue G) > Open settings file, edit, save. The file is `%APPDATA%\G710 keys\g710.ini`, or `g710.ini` next to `g710.exe` if one is there (for running it from a USB stick, say). Changes apply within 30 seconds or on the next M key press. A key is F1-F24, A-Z, 0-9 or `none`, optionally after `Ctrl+`, `Shift+`, `Alt+`, `Win+`.

## How it works

The keyboard has a table saying what each G key types; it resets to 1-6 whenever the keyboard is plugged in or the PC starts. `g710.exe` writes the active set's keys into it on start, after a replug, after the PC wakes, on each M key press, and checks it every 30 seconds. Plain keys (F13) are then sent by the keyboard itself. Keys with Ctrl/Shift/Alt/Win don't fit in that table, so `g710.exe` sends those itself when it sees the G key, as it does for MR. Details: [PROTOCOL.md](PROTOCOL.md).

Quit it from the tray icon. The G keys then keep sending the last set's plain keys until the keyboard is replugged, when they go back to 1-6.

Not tested yet: setting the keys again after the keyboard is replugged or the PC wakes from sleep, and the tray's Uninstall.

Known limits:
- Keys `g710.exe` sends itself (Ctrl combinations, MR) don't reach programs running as administrator, a Windows rule. Plain keys do, since the keyboard sends them.
- Don't run Logitech Gaming Software at the same time: it writes its own G-key table, and the two would undo each other. `g710.exe` shows a warning in the tray if it finds it running. Logitech G Hub doesn't support the G710+ as far as known, so it shouldn't interfere (not tested while it ran).

## Install

1. Download `g710.exe` (Windows 10 or 11, 64-bit) and run it. The exe isn't signed, so Windows may say "Windows protected your PC": click More info, then Run anyway.
2. It asks whether to install:
   - Yes: it copies itself to `%LOCALAPPDATA%\Programs\G710 keys\`, starts with Windows from there, and starts now. The downloaded file can be deleted.
   - No: it runs from where it is, this time only.
3. A blue G appears in the tray; hovering it shows the active set. Clicking it gives the menu: Start at logon, Open settings file, Uninstall, Quit.
4. If Logitech Gaming Software starts with Windows, turn that off: Task Manager > Startup apps > "Launch LCore" > Disable.

No admin rights or driver needed. To update, run the new `g710.exe` and answer Yes: it replaces the installed copy and keeps the settings.

Start at logon is a "G710 keys" entry in the current user's Run key (`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`).

Portable use: put a `g710.ini` next to `g710.exe` (an empty file is enough). It then uses that file for its settings and never offers to install.

## Uninstall

Tray icon > Uninstall. It removes the logon entry, the installed copy and the settings, and puts G1-G6 back to typing 1-6.

## Build

`build.cmd` builds `g710.exe` from `src/g710.c` and `src/g710.rc` (icon and version information) with Visual Studio's C++ tools, found with `vswhere`. No other dependency: Windows' own HID functions.

On GitHub, `.github/workflows/build.yml` builds it on every push (the exe is attached to the run as an artifact). To publish a version: update the version numbers in `src/g710.rc`, commit, then push a tag such as `v1.0.0`; the workflow creates a release of that name with `g710.exe` attached.

The icon comes from `tools/make_icon.ps1` (run it with Windows PowerShell to redraw `src/g710.ico`).

## Folder

- `src/g710.c`: the program; `src/g710.rc` and `src/g710.ico`: its icon and version information.
- `.github/workflows/build.yml`: the GitHub build and release.
- `PROTOCOL.md`: what was found about the keyboard.
- `tools/`: Python scripts used to find it (need `.venv`, made with `uv venv .venv` then `uv pip install --python .venv\Scripts\python.exe hidapi`). `enum.py` and `features.py` only read; `capture.py` and `keywatch.py` log key presses; `gkeys_set.py` and `gkeys_off.py` write the G-key table.
- `captures/`: logs from the tests.

## License

MIT, see [LICENSE](LICENSE).
