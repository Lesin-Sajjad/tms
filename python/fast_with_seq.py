# Create the Python file with the corrected code so the user can download it

import py1401
import numpy as np
import matplotlib.pyplot as plt

VOLT_RANGE = 5.0
BUFFER = 2000
BLOCK = 100

# connect to 1401
u14 = py1401.Create()

if not u14.Open():
    print("Error:", u14.GetError())
    exit()

print("1401 connected")
u14.Write("load emg.seq;")
print(u14.ReadLine())

# load and run sequencer
u14.Write("load emg.seq;")
u14.Write("run;")

# buffers
ch0 = np.zeros(BUFFER)
ch1 = np.zeros(BUFFER)
ch2 = np.zeros(BUFFER)
ch3 = np.zeros(BUFFER)

# plotting
plt.ion()
fig, axes = plt.subplots(4, 1, sharex=True, figsize=(10, 8))

line0, = axes[0].plot(ch0)
line1, = axes[1].plot(ch1)
line2, = axes[2].plot(ch2)
line3, = axes[3].plot(ch3)

axes[0].set_title("ADC0 (EMG)")
axes[1].set_title("ADC1 (Trigger)")
axes[2].set_title("ADC2")
axes[3].set_title("ADC3")

for ax in axes:
    ax.set_ylim(-5, 5)
    ax.set_ylabel("Volts")

axes[3].set_xlabel("Samples")

plt.show(block=False)

try:
    while True:

        # read memory block
        u14.Write(f"memr 0 {BLOCK*4};")
        line = u14.ReadLine()

        if not line.strip():
            continue

        values = np.array(list(map(int, line.split())))

        if len(values) < 4:
            continue

        samples = values.reshape(-1,4)

        volts = samples * (VOLT_RANGE / 32768)

        n = len(volts)

        # shift buffers
        ch0 = np.roll(ch0, -n)
        ch1 = np.roll(ch1, -n)
        ch2 = np.roll(ch2, -n)
        ch3 = np.roll(ch3, -n)

        ch0[-n:] = volts[:, 0]
        ch1[-n:] = volts[:, 1]
        ch2[-n:] = volts[:, 2]
        ch3[-n:] = volts[:, 3]

        # update plots
        line0.set_ydata(ch0)
        line1.set_ydata(ch1)
        line2.set_ydata(ch2)
        line3.set_ydata(ch3)

        fig.canvas.draw_idle()
        fig.canvas.flush_events()

except KeyboardInterrupt:
    print("Stopping...")

finally:
    u14.Write("stop;")
    u14.Close()
    print("Device closed")
