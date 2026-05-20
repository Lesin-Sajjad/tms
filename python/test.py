import py1401
import numpy as np
import matplotlib.pyplot as plt
import time

VOLT_RANGE = 5.0
BUFFER = 500

u14 = py1401.Create()

if not u14.Open():
    print("Error:", u14.GetError())
    exit()

print("1401 connected")

# clear any previous command state
u14.Write("RESET;")
time.sleep(0.1)

ch = np.zeros((4, BUFFER))
idx = 0

plt.ion()
fig, axes = plt.subplots(4,1,sharex=True,figsize=(10,8))

lines = []
titles = ["ADC0 (EMG)", "ADC1 (Trigger)", "ADC2", "ADC3"]

for i in range(4):
    line, = axes[i].plot(ch[i])
    axes[i].set_ylim(-5,5)
    axes[i].set_ylabel("Volts")
    axes[i].set_title(titles[i])
    lines.append(line)

axes[3].set_xlabel("Samples")

try:

    while True:

        # request ADC read
        u14.Write("adc,-3;")

        line = u14.ReadLine()

        if not line:
            continue

        values = list(map(int, line.split()))
        volts = [v*(VOLT_RANGE/32768) for v in values]

        if len(volts) == 4:

            ch[0,idx] = volts[0]
            ch[1,idx] = volts[1]
            ch[2,idx] = volts[2]
            ch[3,idx] = volts[3]

            idx = (idx+1) % BUFFER

        if idx % 10 == 0:

            for i in range(4):
                lines[i].set_ydata(np.roll(ch[i], -idx))

            plt.pause(0.001)

        time.sleep(0.002)   # prevents command overload

except KeyboardInterrupt:
    print("Stopping...")

finally:
    u14.Close()
    print("Device closed")