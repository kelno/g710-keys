# Logitech G710+ protocol notes

> Vibe-coded: these notes were written by Claude (Anthropic's AI) from tests on a real G710+.

Keyboard: USB `046D:C24D`. Findings from one G710+ on Windows 11, 2026-11-19, found by reading the keyboard's HID descriptor and reports (no driver, packet capture or disassembly). "Confirmed" means tested on the keyboard; "guess" means not.

## Interfaces

- Interface 0 (`MI_00`): standard boot keyboard. Windows owns it; programs can't read it.
- Interface 1 (`MI_01`), three collections:
  - `Col01`: a second keyboard (usage page 0x01, usage 6), report ID 1, a 20-byte key array. Windows owns it.
  - `Col02`: consumer control (media keys), report ID 2, 7 bits: 0xB5 next, 0xB6 previous, 0xB7 stop, 0xCD play/pause, 0xE9 volume up, 0xEA volume down, 0xE2 mute.
  - `Col03`: vendor collection, usage page 0xFF00. Any program can open it, read and write, without admin rights or a driver. This is where the G and M keys are reported and where their settings live.

## Vendor collection (`Col03`) reports

| Report ID | Kind | Size (bytes, without ID) | Meaning |
|---|---|---|---|
| 0x03 | input | 3 | G and M keys (confirmed) |
| 0x04 | input | 7 | unknown, never seen |
| 0x05 | feature | 7 | unknown; always read `00 19 49 51 ff ff ff` |
| 0x06 | feature | 1 | M-key lights (confirmed) |
| 0x07 | feature | 4 | unknown; always read `df 07 df 01` |
| 0x08 | feature | 3 | guess: backlight levels; always read `02 03 00` |
| 0x09 | feature | 12 | G-key table (confirmed) |
| 0x80 | feature | 7 | unknown; a read returned report 0x09's first bytes |

Windows pads every feature report of this collection to 13 bytes (the largest); send and read them at that length. `tools/enum.py` prints the raw descriptor; `tools/features.py` reads every feature report (read-only).

## Input report 0x03: G and M keys (confirmed)

Bytes after the report ID:

- Byte 1, bits 0-5: G1 (0x01), G2 (0x02), G3 (0x04), G4 (0x08), G5 (0x10), G6 (0x20).
- Byte 2, bits 4-7: M1 (0x10), M2 (0x20), M3 (0x40), MR (0x80).
- One report per press and one per release; the bits are the keys held at that moment.

The keyboard sends these whether or not Logitech's software runs (`captures/keys-with-lgs.log`, `captures/keys-without-lgs.log`).

## Feature report 0x09: the G-key table (confirmed)

Bytes 1-6 are the HID keyboard usage each of G1-G6 types, sent through the keyboard's normal key path, so Windows and every program see an ordinary key.

- After plugging in: `1e 1f 20 21 22 23`, the usages for 1-6. That's why the G keys type 1-6 (or `&é"'(§` on AZERTY) without software.
- Logitech Gaming Software writes zeros, so the G keys type nothing and it handles them from report 0x03.
- Any usage works; F13-F18 (`68 69 6a 6b 6c 6d`) tested: Windows sees F13-F18 (`captures/keywatch-f13.log`).
- One usage per key, no modifiers: Ctrl+F13 can't be stored.
- Bytes 7-12: writes there read back as zeros, and putting F19-F24 there made the M keys send nothing. The M keys can't be mapped in the keyboard.
- Not stored: the table goes back to 1-6 whenever the keyboard is unplugged or the PC restarts. Something has to write it each time.

## Feature report 0x06: M lights (confirmed)

One byte: 0x10 M1, 0x20 M2, 0x40 M3 (M1-M3 confirmed by g710.exe switching sets); 0x80 should be MR (not tried). Bits combine. The lights never change by themselves: pressing an M key only sends report 0x03, and the software decides what to light. After plugging in: all off.

## What Logitech's software does, as far as seen

Writes zeros to the G-key table, reads report 0x03, does the actions, switches sets on M1-M3, sets the lights. Nothing seen needs its driver or admin rights.
