# G710 keys

> Vibe-coded: the code, this README and the protocol notes were written by Claude AI, and every step was tested on a real G710+.

Makes the Logitech G710+ G1-G6, M1-M3 and MR keys work without Logitech's software, as keys that games and programs (AutoHotkey, PowerToys, OBS, Discord...) can bind. A small tray program for Windows 10 and 11, using about 2 MB of memory where Logitech Gaming Software used 57 MB.

**Download:** [latest release](https://github.com/kelno/g710-keys/releases/latest) (`g710.exe`).

# For users

## Install

1. Download `g710.exe` and run it. The exe isn't signed, so Windows may say "Windows protected your PC": click More info, then Run anyway.
2. It asks whether to install:
   - Yes: it copies itself to `%LOCALAPPDATA%\Programs\G710 keys\`, starts with Windows from there, and starts now. You can delete the downloaded file.
   - No: it runs from where it is, this time only.
3. A blue G appears in the tray. Hovering it shows the active set; clicking it gives the menu: Start at logon, Open settings file, Uninstall, Quit.
4. If Logitech Gaming Software starts with Windows, turn that off: Task Manager > Startup apps > "Launch LCore" > Disable. Running both, they undo each other's settings.

No admin rights or driver needed.

## What the keys do

M1, M2 and M3 choose one of three sets for G1-G6; the chosen M key lights up, and the set is remembered across restarts.

| | Set M1 | Set M2 | Set M3 |
|---|---|---|---|
| G1-G6 | F13-F18 | F19-F24 | Ctrl+F13 to Ctrl+F18 |

MR sends Ctrl+F19 in every set. No real keyboard has F13-F24, so nothing else uses them: bind them in your game or program like any other key.

## Change the keys

Tray icon > Open settings file, edit, save. Changes apply within 30 seconds or on the next M key press. A key is F1-F24, A-Z, 0-9 or `none`, optionally after `Ctrl+`, `Shift+`, `Alt+`, `Win+`, for example:

```ini
[M1]
G1=F13
G2=Ctrl+Shift+A
```

The file is `%APPDATA%\G710 keys\g710.ini`.

## Update

Download the new `g710.exe`, run it and answer Yes: it replaces the installed copy and keeps your settings.

## Uninstall

Tray icon > Uninstall. It removes the start-with-Windows entry, the installed copy and the settings, and puts G1-G6 back to typing 1-6.

## Good to know

- Keys with Ctrl/Shift/Alt/Win, and MR, don't reach programs running as administrator (a Windows rule). Plain keys like F13 do.
- After Quit, the G keys keep sending the current set's plain keys until the keyboard is unplugged or the PC restarts; then they type 1-6 again.
- Logitech G Hub doesn't support the G710+ as far as known, so it shouldn't interfere (not tested while it ran).
- Portable use: put a `g710.ini` next to `g710.exe` (an empty file is enough). It then keeps its settings there and never offers to install.
- Start with Windows is a "G710 keys" entry in the current user's Run key (`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`).

# For developers

## How it works

The keyboard has a table saying what each G key types (vendor feature report 0x09); it resets to 1-6 whenever the keyboard is plugged in or the PC starts. `g710.exe` writes the active set's keys into it on start, after a replug, after the PC wakes, on each M key press, and checks it every 30 seconds. Plain keys (F13) are then sent by the keyboard itself. Keys with Ctrl/Shift/Alt/Win don't fit in that table, so `g710.exe` sends those itself (`SendInput`) when it reads the G key from the keyboard's vendor collection, as it does for MR. The M lights are feature report 0x06. Everything found about the keyboard: [PROTOCOL.md](PROTOCOL.md).

Tested on a real G710+: every key in every set, setting the keys again after a replug, install, update and Uninstall. Not tested yet: setting them again after the PC wakes from sleep.

## Build

`build.cmd` builds `g710.exe` from `src/g710.c` and `src/g710.rc` (icon and version information) with Visual Studio's C++ tools, found with `vswhere`. No other dependency: Windows' own HID functions.

## Release

`.github/workflows/build.yml` builds on every push (the exe is attached to the run as an artifact). To publish a version: update the version numbers in `src/g710.rc`, commit, then push a tag such as `v1.0.0`; the workflow creates a release of that name with `g710.exe` attached.

## Folder

- `src/g710.c`: the program; `src/g710.rc` and `src/g710.ico`: its icon and version information.
- `.github/workflows/build.yml`: the GitHub build and release.
- `PROTOCOL.md`: what was found about the keyboard.
- `tools/`: scripts used to find it. The Python ones need `.venv` (`uv venv .venv`, then `uv pip install --python .venv\Scripts\python.exe hidapi`): `enum.py` and `features.py` only read; `capture.py` and `keywatch.py` log key presses; `gkeys_set.py` and `gkeys_off.py` write the G-key table. `make_icon.ps1` redraws `src/g710.ico` (run it with Windows PowerShell).
- `captures/`: logs from the tests.

# License

MIT, see [LICENSE](LICENSE).
