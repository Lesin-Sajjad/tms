import py1401
import time

VOLT_RANGE = 5.0

u14 = py1401.Create()

if not u14.Open():
    print("Error:", u14.GetError())
    exit()

print("1401 connected")
print("ADC0 = EMG amplifier output")
print("ADC1 = Trigger output")
print("ADC2 = unused")
print("ADC3 = unused\n")

try:
    while True:

        # read ADC channels 0–3
        u14.Write("adc,-3;")
        line = u14.ReadLine()

        values = list(map(int, line.split()))

        # convert to volts
        volts = [v * (VOLT_RANGE / 32768) for v in values]

        if len(values) >= 4:
            print(
                f"EMG (ADC0): {values[0]:6d} ({volts[0]:6.3f} V)   "
                f"Trigger (ADC1): {values[1]:6d} ({volts[1]:6.3f} V)   "
                f"ADC2: {values[2]:6d} ({volts[2]:6.3f} V)   "
                f"ADC3: {values[3]:6d} ({volts[3]:6.3f} V)"
            )
        else:
            print("Returned:", line)

        time.sleep(0.05)

except KeyboardInterrupt:
    print("\nStopping...")

finally:
    u14.Close()
    print("Device closed")