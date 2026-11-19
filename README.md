# G710+ keys

> Vibe-coded: the code, this README and the protocol notes were written by Claude AI, and every step was tested on a real G710+.

Makes the Logitech G710+ G1-G6, M1-M3 and MR keys work without Logitech's software, as keys that games and programs (AutoHotkey, PowerToys, OBS, Discord...) can bind. A small tray program for Windows 10 and 11, using about 2 MB of memory where Logitech Gaming Software used 57 MB.

**Download:** [latest release](https://github.com/kelno/g710-keys/releases/latest) (`g710.exe`).

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

## Check your download (optional)

Each release is built by GitHub from this repo's code, and GitHub signs a public record of which exe came from which commit. To check that your `g710.exe` is that exe, with the [GitHub CLI](https://cli.github.com/):

```
gh attestation verify g710.exe --repo kelno/g710-keys
```

## Development

Building, releasing and how it works: [DEVELOPMENT.md](DEVELOPMENT.md). What was found about the keyboard: [PROTOCOL.md](PROTOCOL.md).

## License

MIT, see [LICENSE](LICENSE).
