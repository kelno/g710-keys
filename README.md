# G710 keys

*[Written by Claude AI on Kelno's behalf; Kelno reviewed and edited it. If you'd rather not receive AI-assisted text, just say so, no problem.]*

The code, the notes and the protocol findings were made by Claude (Anthropic's AI) working with Kelno, who tested every step on the keyboard.

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

Known limits:
- Keys `g710.exe` sends itself (Ctrl combinations, MR) don't reach programs running as administrator, a Windows rule. Plain keys do, since the keyboard sends them.
- Don't run Logitech Gaming Software at the same time: it writes its own G-key table, and the two would undo each other. `g710.exe` shows a warning in the tray if it finds it running. Logitech G Hub doesn't support the G710+ as far as known, so it shouldn't interfere (not tested while it ran).

## Install

1. Download `g710.exe` (Windows 10 or 11, 64-bit) and put it wherever you like.
2. Run it. A blue G appears in the tray; hovering it shows the active set.
3. The first time, it asks whether to start with Windows. You can change that later in the tray menu (Start at logon), which adds or removes a "G710 keys" entry in the current user's Run key (`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`). If you move `g710.exe`, run it once from its new place and the entry follows.
4. If Logitech Gaming Software starts with Windows, turn that off: Task Manager > Startup apps > "Launch LCore" > Disable.

No admin rights, driver or installer needed.

## Uninstall

Tray icon > untick Start at logon, then Quit. Delete `g710.exe` and the `%APPDATA%\G710 keys` folder. Replug the keyboard (or restart) and the G keys type 1-6 again.

## Build

`build.cmd` builds `g710.exe` from `src/g710.c` and `src/g710.rc` (version information) with Visual Studio's C++ tools (path set at the top of `build.cmd`). No other dependency: Windows' own HID functions.

## Folder

- `src/g710.c`: the program; `src/g710.rc`: its version information.
- `PROTOCOL.md`: what was found about the keyboard.
- `tools/`: Python scripts used to find it (need `.venv`, made with `uv venv .venv` then `uv pip install --python .venv\Scripts\python.exe hidapi`). `enum.py` and `features.py` only read; `capture.py` and `keywatch.py` log key presses; `gkeys_set.py` and `gkeys_off.py` write the G-key table.
- `captures/`: logs from the tests.

## License

MIT, see [LICENSE](LICENSE).
