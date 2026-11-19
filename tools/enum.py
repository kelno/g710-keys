import hid
for d in hid.enumerate(0x046D, 0xC24D):
    print(f"if={d['interface_number']} page=0x{d['usage_page']:04X} usage=0x{d['usage']:04X} path={d['path'].decode()}")
    try:
        h = hid.device(); h.open_path(d['path'])
        try:
            desc = h.get_report_descriptor()
            print("  descriptor:", bytes(desc).hex(' '))
        except Exception as e:
            print("  no descriptor:", e)
        h.close()
    except Exception as e:
        print("  open failed:", e)
