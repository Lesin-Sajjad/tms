import py1401
import numpy as np
import matplotlib.pyplot as plt
import time

# --- Configuration ---
VOLT_RANGE = 5.0
BUFFER_SIZE = 2000  # Number of points to show on screen
POLL_INTERVAL = 0.001 # Target polling interval

u14 = py1401.Create()

if not u14.Open():
    print("Error: Could not open 1401. Ensure it is connected and powered on.")
    exit()

print("1401 connected. Starting superimposed real-time plot...")

# Data Buffers
data = np.zeros((4, BUFFER_SIZE))
idx = 0

# Setup Plotting
plt.ion()
fig, ax = plt.subplots(figsize=(10, 6))

colors = ['blue', 'green', 'orange', 'red']
labels = ["ADC0 (EMG1)", "ADC1 (EMG2)", "ADC2 (EMG3)", "ADC3 (TMS)"]
lines = []

# Initialize 4 lines on the same axis
for i in range(4):
    line, = ax.plot(np.zeros(BUFFER_SIZE), color=colors[i], label=labels[i], linewidth=1)
    lines.append(line)

ax.set_ylim(-VOLT_RANGE, VOLT_RANGE)
ax.set_xlabel("Time (seconds relative to now)")
ax.set_ylabel("Voltage (Volts)")
ax.set_title("TMS Superimposed Real-Time Monitor")
ax.grid(True, alpha=0.3)
ax.legend(loc='upper right')

# Calculate an approximate time axis (this will update based on measured rate)
# We assume a fixed width of points for the X-axis visualization
x_axis = np.linspace(-1.0, 0, BUFFER_SIZE) 

start_time = time.time()
counter = 0

try:
    while True:
        # 1. Poll Hardware
        u14.Write("adc,-3;")
        line_data = u14.ReadLine()

        if not line_data:
            continue

        vals = line_data.split()

        if len(vals) == 4:
            # 2. Convert and Store
            for i in range(4):
                data[i, idx] = int(vals[i]) * VOLT_RANGE / 32768.0
            
            idx = (idx + 1) % BUFFER_SIZE
            counter += 1

        # 3. Update Plot (occasionally to save CPU)
        if counter % 50 == 0:
            # Shift data so the latest point is on the right
            for i in range(4):
                lines[i].set_ydata(np.roll(data[i], -idx))
                lines[i].set_xdata(x_axis)

            fig.canvas.draw_idle()
            plt.pause(0.001)

        # 4. Print stats
        if counter % 1000 == 0:
            elapsed = time.time() - start_time
            rate = counter / elapsed
            # Update X-axis label to reflect actual time window based on rate
            window_sec = BUFFER_SIZE / rate
            x_axis = np.linspace(-window_sec, 0, BUFFER_SIZE)
            print(f"Sampling Rate: {rate:.0f} Hz | Window: {window_sec:.2f}s")

except KeyboardInterrupt:
    print("\nStopping...")

finally:
    u14.Close()
    print("Device closed.")
