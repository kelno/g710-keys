# Sets the G1-G6 key table (feature report 0x09) to the given HID keyboard usage codes, keeping bytes 7-12 as read.
# Default: F13-F18 (0x68-0x6D). "off" writes zeros. Volatile: the keyboard restores 1-6 when replugged.
import sys, hid
codes = [0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D]
if len(sys.argv) > 1:
    codes = [0] * 6 if sys.argv[1] == 'off' else [int(x, 16) for x in sys.argv[1:7]]
path = next(d['path'] for d in hid.enumerate(0x046D, 0xC24D) if d['usage_page'] == 0xFF00)
h = hid.device(); h.open_path(path)
before = bytes(h.get_feature_report(0x09, 13))
print("before:", before.hex(' '))
h.send_feature_report(bytes([0x09] + codes) + before[7:13])
print("after: ", bytes(h.get_feature_report(0x09, 13)).hex(' '))
h.close()
