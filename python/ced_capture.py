import numpy as np
import matplotlib.pyplot as plt
import time
import os
import sys

# --- Load the Official CED Module ---
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.append(script_dir)

# For Python 3.8+ on Windows
if sys.platform == 'win32' and hasattr(os, 'add_dll_directory'):
    os.add_dll_directory(script_dir)

try:
    import py1401
except ImportError as e:
    print(f"Error: Could not load 'py1401'. {e}")
    sys.exit(1)

# --- Configuration ---
NUM_CHANNELS = 4         # 0=EMG1, 1=EMG2, 2=EMG3, 3=TMS Trigger
SAMPLING_RATE_HZ = 10000 # 10kHz per channel
CAPTURE_DURATION_S = 0.5 # 500ms
NUM_SAMPLES_PER_CHAN = int(SAMPLING_RATE_HZ * CAPTURE_DURATION_S)
VOLTAGE_RANGE = 5.0      # Typical for Micro 1401

def capture_tms_emg():
    u14 = py1401.Create()
    if not u14.Open():
        err_code, err_msg = u14.GetError()
        print(f"Failed to open the 1401: {err_msg} (Error {err_code})")
        return

    try:
        print(f"Successfully opened Micro 1401.")
        
        # 1. Load ADCMEM command
        if not u14.Load('adcmem', r'C:\1401'):
            print("Note: 'adcmem' command loading status checked.")

        # 2. Setup Sampling
        point_size = 2 # 2 bytes per sample (16-bit)
        total_points = NUM_SAMPLES_PER_CHAN * NUM_CHANNELS
        block_size_bytes = total_points * point_size
        
        # Clock: 10kHz = 1 tick every 100us
        # divide_pre * divide_count = 100 (1MHz / 100 = 10kHz)
        divide_pre = 10
        divide_count = 10

        # Command: adcmem,i,pointsize,blockstart,blocksize,startchan,endchan,clock,pre,count
        cmd = f"adcmem,i,{point_size},0,{block_size_bytes},0,{NUM_CHANNELS-1},c,{divide_pre},{divide_count}"
        print(f"Starting 4-channel capture (10kHz/chan)...")
        
        if not u14.Write(cmd):
            print(f"Capture command failed: {u14.GetError()[1]}")
            return

        # 3. Wait for data to fill 1401 memory
        time.sleep(CAPTURE_DURATION_S + 0.1)

        # 4. Read data to host
        # The data comes back interleaved: [Ch0, Ch1, Ch2, Ch3, Ch0, Ch1...]
        raw_data = np.zeros(total_points, dtype=np.int16)
        u14.ToHost(raw_data, 0) # Read from 1401 memory area 0

        # 5. De-interleave and Scale
        # Reshape to (Samples, Channels)
        data_matrix = raw_data.reshape((NUM_SAMPLES_PER_CHAN, NUM_CHANNELS))
        volts_matrix = data_matrix * (VOLTAGE_RANGE / 32768.0)

        # 6. Plotting
        time_axis = np.linspace(0, CAPTURE_DURATION_S, NUM_SAMPLES_PER_CHAN)
        fig, axes = plt.subplots(NUM_CHANNELS, 1, figsize=(12, 10), sharex=True)
        labels = ['EMG 1 (ADC 0)', 'EMG 2 (ADC 1)', 'EMG 3 (ADC 2)', 'TMS Trigger (ADC 3)']
        colors = ['blue', 'green', 'orange', 'red']

        for i in range(NUM_CHANNELS):
            axes[i].plot(time_axis, volts_matrix[:, i], color=colors[i], linewidth=0.8)
            axes[i].set_ylabel("Volts")
            axes[i].set_title(labels[i])
            axes[i].grid(True, alpha=0.3)

        plt.xlabel("Time (seconds)")
        plt.tight_layout()
        print("Displaying captured signals...")
        plt.show()

    finally:
        u14.Write('adcmem,k') # Stop sampling command
        u14.Close()
        print("Device closed.")

if __name__ == "__main__":
    capture_tms_emg()
