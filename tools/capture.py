# Prints and logs every input report from the G710+ vendor and consumer collections.
import hid, sys, time, threading
LOG = sys.argv[1]
SECONDS = float(sys.argv[2]) if len(sys.argv) > 2 else 120
out = open(LOG, 'w', buffering=1)
def watch(d, label):
    h = hid.device(); h.open_path(d['path']); h.set_nonblocking(False)
    end = time.time() + SECONDS
    while time.time() < end:
        r = h.read(64, 500)
        if r:
            line = f"{time.time():.3f} {label} {bytes(r).hex(' ')}"
            print(line); out.write(line + '\n')
for d in hid.enumerate(0x046D, 0xC24D):
    if d['usage_page'] in (0xFF00, 0x000C):
        threading.Thread(target=watch, args=(d, 'vendor' if d['usage_page'] == 0xFF00 else 'consumer'), daemon=True).start()
print(f"Capturing for {SECONDS:.0f} s. Press G1..G6, then M1, M2, M3, MR, one at a time.")
time.sleep(SECONDS + 1)
print("Done.")
