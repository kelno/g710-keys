# G710+ keys: development

> Vibe-coded: the code and these notes were written by Claude AI, and every step was tested on a real G710+.

How to use the program: [README.md](README.md). Everything found about the keyboard: [PROTOCOL.md](PROTOCOL.md).

## How it works

The keyboard has a table saying what each G key types (vendor feature report 0x09); it resets to 1-6 whenever the keyboard is plugged in or the PC starts. `g710.exe` writes the active set's keys into it on start, after a replug, after the PC wakes, on each M key press, and checks it every 30 seconds. Plain keys (F13) are then sent by the keyboard itself. Keys with Ctrl/Shift/Alt/Win don't fit in that table, so `g710.exe` sends those itself (`SendInput`) when it reads the G key from the keyboard's vendor collection, as it does for MR. The M lights are feature report 0x06.

Tested on a real G710+: every key in every set, setting the keys again after a replug, install, update and Uninstall. Not tested yet: setting them again after the PC wakes from sleep.

## Build

`build.cmd` builds `g710.exe` from `src/g710.c` and `src/g710.rc` (icon and version information) with Visual Studio's C++ tools, found with `vswhere`. No other dependency: Windows' own HID functions.

## Release

`.github/workflows/build.yml` builds on every push (the exe is attached to the run as an artifact). To publish a version: update the version numbers in `src/g710.rc`, commit, then push a tag such as `v1.0.3`; the workflow creates a release of that name with `g710.exe` attached and its SHA-256 in the notes.

On every push, the workflow also signs a build attestation (`actions/attest-build-provenance`): a public, Sigstore-signed record that this exact exe was built by this workflow from that commit. `gh attestation verify g710.exe --repo kelno/g710-keys` checks a download against it. The exe isn't bit-for-bit reproducible (the Microsoft compiler stamps build details into it), so a local build won't match a release's hash; the attestation is what ties a release to the code.

## Folder

- `src/g710.c`: the program; `src/g710.rc` and `src/g710.ico`: its icon and version information.
- `.github/workflows/build.yml`: the GitHub build and release.
- `PROTOCOL.md`: what was found about the keyboard.
- `tools/`: scripts used to find it. The Python ones need `.venv` (`uv venv .venv`, then `uv pip install --python .venv\Scripts\python.exe hidapi`): `enum.py` and `features.py` only read; `capture.py` and `keywatch.py` log key presses; `gkeys_set.py` and `gkeys_off.py` write the G-key table. `make_icon.ps1` redraws `src/g710.ico` (run it with Windows PowerShell).
- `captures/`: logs from the tests.
