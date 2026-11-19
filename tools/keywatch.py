# Shows which keys Windows sees pressed, by name, until the window is closed or the time runs out.
import ctypes, sys, time
SECONDS = float(sys.argv[1]) if len(sys.argv) > 1 else 300
LOG = sys.argv[2] if len(sys.argv) > 2 else None
user32 = ctypes.windll.user32
names = {vk: f"F{vk - 0x70 + 1}" for vk in range(0x70, 0x88)}
names.update({vk: chr(vk) for vk in range(0x30, 0x3A)})
names.update({0x11: "Ctrl", 0x10: "Shift", 0x12: "Alt"})
out = open(LOG, 'w', buffering=1) if LOG else None
down = set()
print(f"Watching keys for {SECONDS:.0f} s. Press the G keys.")
end = time.time() + SECONDS
while time.time() < end:
    for vk in names:
        pressed = bool(user32.GetAsyncKeyState(vk) & 0x8000)
        if pressed and vk not in down:
            down.add(vk); line = f"{time.strftime('%H:%M:%S')} {names[vk]} down"
            print(line); out and out.write(line + '\n')
        elif not pressed and vk in down:
            down.discard(vk)
    time.sleep(0.01)
