import py1401
import numpy as np
import matplotlib.pyplot as plt
import time

VOLT_RANGE = 5.0
NUMSAMP = 2000  # Number of samples to read per block transfer

u14 = py1401.Create()
if not u14.Open():
    print("Error:", u14.GetError())
    exit()

print("1401 connected")

# Setup plot
plt.ion()
fig, axes = plt.subplots(4, 1, sharex=True, figsize=(10, 8))
lines = []
titles = ["ADC0 (EMG)", "ADC1 (Trigger)", "ADC2", "ADC3"]

# Initialize empty plots with 0s
for i in range(4):
    line, = axes[i].plot(np.zeros(NUMSAMP))
    axes[i].set_ylim(-5, 5)
    axes[i].set_ylabel("Volts")
    axes[i].set_title(titles[i])
    lines.append(line)
axes[3].set_xlabel("Samples")

start = time.time()
samples_total = 0

try:
    # 1. Load the ADCMEM command into the 1401
    u14.Load("", "kill,adcmem")

    while True:
        # 2. Instruct 1401 to capture a block of data into its memory
        # NOTE: This string mirrors the C++ setup. You may need to adjust the 
        # arguments (like "0 1,1") depending on your specific 4-channel needs.
        setup_cmd = f"ADCMEM,I,2,0,{NUMSAMP * 2},0 1,1,C,10,10;"
        u14.Write(setup_cmd)
        
        # 3. Poll for hardware completion (wait until memory buffer is full)
        while True:
            u14.Write("ADCMEM,?;")
            status = u14.LongsFrom1401(1) 
            # Check if status returned 0 (job done)
            if status and status[0] == 0:
                break
            time.sleep(0.001) # Small yield to prevent CPU hogging

        # 4. Transfer the binary block to the host PC
        # 16-bit integers = 2 bytes per sample
        buffer_size = NUMSAMP * 2 
        raw_data = bytearray(buffer_size)
        
        # Read directly from 1401 memory to host byte buffer
        u14.ToHost(raw_data, buffer_size, 0, 0)
        
        # 5. Fast binary parsing with NumPy
        # Convert raw binary to int16, then convert to Volts
        int_data = np.frombuffer(raw_data, dtype=np.int16)
        volt_data = int_data * (VOLT_RANGE / 32768.0)

        # Update plots (Example: just updating ADC0 for simplicity in this snippet. 
        # You will need to slice volt_data based on how ADCMEM multiplexes your channels)
        lines[0].set_ydata(volt_data)
        
        fig.canvas.draw_idle()
        plt.pause(0.001)

        samples_total += NUMSAMP
        
        # Print real sampling rate
        if samples_total % (NUMSAMP * 5) == 0:
            elapsed = time.time() - start
            print(f"Sampling rate: {samples_total / elapsed:.0f} Hz")

except KeyboardInterrupt:
    print("Stopping...")

finally:
    u14.Close()
    print("Device closed")