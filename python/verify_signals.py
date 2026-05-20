import numpy as np
import time
import os
import sys

# --- Load the Official CED Module ---
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.append(script_dir)

if sys.platform == 'win32' and hasattr(os, 'add_dll_directory'):
    os.add_dll_directory(script_dir)

try:
    import py1401
except ImportError as e:
    print(f"Error: Could not load 'py1401'. {e}")
    sys.exit(1)

def verify_live_signals():
    u14 = py1401.Create()
    if not u14.Open():
        print("Failed to open the 1401.")
        return

    try:
        print("Live Signal Verification (Ctrl+C to stop)")
        print("ADC 0 (EMG1) | ADC 1 (EMG2) | ADC 2 (EMG3) | ADC 3 (TMS)")
        print("-" * 60)
        
        u14.Load('adcmem', r'C:\1401')

        # We'll just read a small burst every 0.5 seconds
        num_chans = 4
        samples_per_chan = 10
        total_samples = num_chans * samples_per_chan
        
        while True:
            # Command to capture a tiny burst
            # adcmem,i,pointsize,start,size,startchan,endchan,clock,pre,count
            # 1kHz sampling for verification is plenty (1000us = 10*100)
            u14.Write(f"adcmem,i,2,0,{total_samples*2},0,3,c,10,100")
            
            # Wait for capture to finish (10ms)
            time.sleep(0.02)
            
            raw_data = np.zeros(total_samples, dtype=np.int16)
            u14.ToHost(raw_data, 0)
            
            # Get the average of the 10 samples per channel to reduce jitter
            data_matrix = raw_data.reshape((samples_per_chan, num_chans))
            averages = np.mean(data_matrix, axis=0) * (5.0 / 32768.0)
            
            # Print values in one line
            print(f"{averages[0]:>11.3f}V | {averages[1]:>11.3f}V | {averages[2]:>11.3f}V | {averages[3]:>11.3f}V", end='\r')
            
            time.sleep(0.3)

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        u14.Write('adcmem,k')
        u14.Close()

if __name__ == "__main__":
    verify_live_signals()
