import pandas as pd
import matplotlib.pyplot as plt
import os

# --- CONFIGURATION ---
file_baseline = 'tcp-throughput.csv'      # Your No-Cut File
file_cut = 'CUT-tcp-throughput.csv'       # Your Fiber-Cut File
cut_time = 12.0                           # When the cut happens
bin_size = 0.5                            # 0.5s intervals (smaller = spikier, larger = smoother)

def get_throughput_series(filename, bin_size):
    """
    Reads the raw packet log and converts it to Throughput (Mbps)
    """
    if not os.path.exists(filename):
        print(f"Warning: {filename} not found.")
        return None

    # 1. Load Data
    df = pd.read_csv(filename)
    
    # 2. Create a Time Bin column (e.g., 0.0, 0.5, 1.0, 1.5...)
    # We floor the exact packet time to the nearest bin
    df['TimeBin'] = (df['Time'] // bin_size) * bin_size
    
    # 3. Group by Bin and Sum Bytes
    grouped = df.groupby('TimeBin')['Bytes'].sum().reset_index()
    
    # 4. Convert to Mbps
    # Formula: (Sum Bytes * 8 bits/byte) / (1,000,000 bits/Mb) / (Bin Size seconds)
    grouped['Throughput_Mbps'] = (grouped['Bytes'] * 8) / 1e6 / bin_size
    
    return grouped

# --- MAIN PROCESSING ---
df_base = get_throughput_series(file_baseline, bin_size)
df_cut = get_throughput_series(file_cut, bin_size)

plt.figure(figsize=(12, 6))

# Plot Baseline (No Cut)
if df_base is not None:
    plt.plot(df_base['TimeBin'], df_base['Throughput_Mbps'], 
             label='Baseline (No Cut)', color='blue', linestyle='--', alpha=0.7)

# Plot Fiber Cut Scenario
if df_cut is not None:
    plt.plot(df_cut['TimeBin'], df_cut['Throughput_Mbps'], 
             label='Fiber Cut Scenario', color='red', linewidth=2)

# Visuals
plt.axvline(x=cut_time, color='black', linestyle=':', label='Fiber Cut Event')
plt.title(f'TCP Throughput Comparison (Bin Size: {bin_size}s)')
plt.xlabel('Time (seconds)')
plt.ylabel('Throughput (Mbps)')
plt.legend()
plt.grid(True, which='both', linestyle='--', linewidth=0.5)

# Save and Show
output_filename = 'comparison_throughput.png'
plt.savefig(output_filename, dpi=300)
print(f"Plot saved to {output_filename}")
plt.show()