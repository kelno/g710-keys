# Read-only: GET every feature report the vendor collection declares.
import hid
SIZES = {5: 7, 6: 1, 7: 4, 8: 3, 9: 12, 0x80: 7}
path = next(d['path'] for d in hid.enumerate(0x046D, 0xC24D) if d['usage_page'] == 0xFF00)
h = hid.device(); h.open_path(path)
for rid, n in SIZES.items():
    try:
        data = h.get_feature_report(rid, n + 1)
        print(f"feature 0x{rid:02X}: {bytes(data).hex(' ')}")
    except Exception as e:
        print(f"feature 0x{rid:02X}: error {e}")
h.close()
