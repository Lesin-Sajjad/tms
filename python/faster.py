import py1401
import numpy as np
import matplotlib.pyplot as plt
import time

VOLT_RANGE = 5.0
BUFFER = 6000
PLOT_INTERVAL = 300

u14 = py1401.Create()

if not u14.Open():
    print("Error:", u14.GetError())
    exit()

print("1401 connected")

# circular buffer
data = np.zeros((4, BUFFER))
idx = 0
counter = 0

plt.ion()
fig, axes = plt.subplots(4,1,sharex=True,figsize=(10,8))

lines = []
titles = ["ADC0 (EMG)", "ADC1 (Trigger)", "ADC2", "ADC3"]

for i in range(4):
    line, = axes[i].plot(data[i])
    axes[i].set_ylim(-5,5)
    axes[i].set_ylabel("Volts")
    axes[i].set_title(titles[i])
    lines.append(line)

axes[3].set_xlabel("Samples")

start = time.time()
samples = 0

try:

    while True:

        # fastest possible command polling
        u14.Write("adc,-3;")
        line = u14.ReadLine()

        if not line:
            continue

        vals = line.split()

        if len(vals) == 4:

            # convert to volts
            v0 = int(vals[0]) * VOLT_RANGE / 32768
            v1 = int(vals[1]) * VOLT_RANGE / 32768
            v2 = int(vals[2]) * VOLT_RANGE / 32768
            v3 = int(vals[3]) * VOLT_RANGE / 32768

            data[0,idx] = v0
            data[1,idx] = v1
            data[2,idx] = v2
            data[3,idx] = v3

            idx = (idx + 1) % BUFFER
            samples += 1

        counter += 1

        # plot only occasionally
        if counter % PLOT_INTERVAL == 0:

            for i in range(4):
                lines[i].set_ydata(np.roll(data[i], -idx))

            fig.canvas.draw_idle()
            plt.pause(0.001)

        # print real sampling rate
        if samples % 5000 == 0:
            elapsed = time.time() - start
            rate = samples / elapsed
            print(f"Polling rate: {rate:.0f} Hz")

except KeyboardInterrupt:
    print("Stopping...")

finally:
    u14.Close()
    print("Device closed")