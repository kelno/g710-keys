# Writes zeros into feature report 0x09 (the G1-G6 key table), as LGS does, then reads it back.
# Volatile: the keyboard restores 1-6 when replugged.
import hid
path = next(d['path'] for d in hid.enumerate(0x046D, 0xC24D) if d['usage_page'] == 0xFF00)
h = hid.device(); h.open_path(path)
print("before:", bytes(h.get_feature_report(0x09, 13)).hex(' '))
h.send_feature_report(bytes([0x09, 0, 0, 0, 0, 0, 0, 0xff, 0, 0, 0, 0, 0]))
print("after: ", bytes(h.get_feature_report(0x09, 13)).hex(' '))
h.close()
