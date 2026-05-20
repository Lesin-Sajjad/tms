import py1401
import numpy as np
import matplotlib.pyplot as plt
import time

VOLT_RANGE = 5.0
BUFFER = 300

# connect to 1401
u14 = py1401.Create()

if not u14.Open():
    print("Error:", u14.GetError())
    exit()

print("1401 connected")

# buffers
ch0 = np.zeros(BUFFER)
ch1 = np.zeros(BUFFER)
ch2 = np.zeros(BUFFER)
ch3 = np.zeros(BUFFER)

plt.ion()

fig, axes = plt.subplots(4, 1, sharex=True, figsize=(10,8))

line0, = axes[0].plot(ch0)
line1, = axes[1].plot(ch1)
line2, = axes[2].plot(ch2)
line3, = axes[3].plot(ch3)

axes[0].set_title("ADC0 (EMG)")
axes[1].set_title("ADC1 (Trigger)")
axes[2].set_title("ADC2")
axes[3].set_title("ADC3")

for ax in axes:
    ax.set_ylim(-5,5)
    ax.set_ylabel("Volts")
axes[0].set_ylim(-5,5)
axes[1].set_ylim(-5, 5)
axes[2].set_ylim(-5,5)
axes[3].set_ylim(-5, 5)

axes[3].set_xlabel("Samples")

try:

    while True:

        # read ADC0–ADC3
        u14.Write("adc,-3;")
        line = u14.ReadLine()

        values = list(map(int, line.split()))
        volts = [v * (VOLT_RANGE / 32768) for v in values]

        # shift buffers
        ch0[:-1] = ch0[1:]
        ch1[:-1] = ch1[1:]
        ch2[:-1] = ch2[1:]
        ch3[:-1] = ch3[1:]

        if len(volts) >= 4:
            ch0[-1] = volts[0]
            ch1[-1] = volts[1]
            ch2[-1] = volts[2]
            ch3[-1] = volts[3]

        # update plots
        line0.set_ydata(ch0)
        line1.set_ydata(ch1)
        line2.set_ydata(ch2)
        line3.set_ydata(ch3)

        plt.pause(0.01)

except KeyboardInterrupt:
    print("Stopping...")

finally:
    u14.Close()
    print("Device closed")