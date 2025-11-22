#!/usr/bin/env python3

import sys
import os

# Check if running as root for /dev/input access
if os.geteuid() != 0:
    print("ERROR: This script requires root (sudo) to read touchscreen input")
    sys.exit(1)

try:
    import evdev
    from evdev import ecodes, InputDevice
except ImportError:
    print("ERROR: evdev library not installed. Install with: pip install evdev")
    sys.exit(1)

class Unbuffered(object):
    def __init__(self, stream):
        self.stream = stream

    def write(self, data):
        self.stream.write(data)
        self.stream.flush()

    def writelines(self, datas):
        self.stream.writelines(datas)
        self.stream.flush()

    def __getattr__(self, attr):
        return getattr(self.stream, attr)

def find_touchscreen():
    """Find and return the first touchscreen device"""
    devices = [InputDevice(path) for path in evdev.list_devices()]
    for device in devices:
        caps = device.capabilities()
        if ecodes.EV_ABS in caps:
            abs_caps = dict(caps[ecodes.EV_ABS])
            if ecodes.ABS_MT_POSITION_X in abs_caps:
                print(f"Found Touchscreen: {device.name} ({device.path})", file=sys.stderr)

                # Get hardware resolution for reference
                x_max = abs_caps[ecodes.ABS_MT_POSITION_X].max
                y_max = abs_caps[ecodes.ABS_MT_POSITION_Y].max
                print(f"Hardware resolution: {x_max}x{y_max}", file=sys.stderr)
                return device, x_max, y_max
    return None, 0, 0

def monitor_touchscreen():
    """Monitor touchscreen and print x,y coordinates to stdout"""
    device, x_max, y_max = find_touchscreen()
    if not device:
        print("Error: No touchscreen device found", file=sys.stderr)
        return

    # Current slot state
    current_slot_id = 0
    current_slot = None
    slots = {}  # store x, y per slot

    print("Starting touch monitoring...", file=sys.stderr)
    print("Format: slot_id,x,y", file=sys.stderr)

    try:
        for event in device.read_loop():
            if event.type == ecodes.EV_ABS:

                # Ensure slot exist
                if current_slot_id not in slots:
                    slots[current_slot_id] = {'x': 0, 'y': 0, 'id': 0}

                current_slot = slots[current_slot_id]

                if event.code == ecodes.ABS_MT_SLOT:
                    current_slot_id = event.value
                elif event.code == ecodes.ABS_MT_POSITION_X:
                    current_slot['x'] = event.value
                    print(f"{current_slot_id}: x={current_slot['x']}, y={current_slot['y']}")
                elif event.code == ecodes.ABS_MT_POSITION_Y:
                    current_slot['y'] = event.value
                    print(f"{current_slot_id}: x={current_slot['x']}, y={current_slot['y']}")
                elif event.code == ecodes.ABS_MT_TRACKING_ID:
                    current_slot['id'] = event.value
                    print(f"{current_slot_id}: {event.value if event.value != -1 else "release"}")

            elif event.type == ecodes.EV_SYN and event.code == ecodes.SYN_REPORT:
                pass

    except KeyboardInterrupt:
        print("\nStopping...", file=sys.stderr)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)


if __name__ == "__main__":
    sys.stdout = Unbuffered(sys.stdout)
    monitor_touchscreen()
