import pandas as pd
import matplotlib.pyplot as plt
# Configuration
MSS = 1024        # Bytes to Segments
TARGET_NODE = 400 
def get_node_data(file_path, node_id):
    """
    Reads file and filters for a specific node to get a clean signal.
    """
    df = pd.read_csv(file_path)
    # Filter for the target node only
    df = df[df['NodeId'] == node_id]
    # Sort by time
    df = df.sort_values('Time')
    return df
# --- Load Data ---
df_base = get_node_data('cwnd-trace.csv', TARGET_NODE)
df_cut  = get_node_data('CUT-cwnd-trace.csv', TARGET_NODE)
# --- Plotting ---
plt.figure(figsize=(12, 6))
# 1. Plot Baseline (Blue Line)
plt.plot(df_base['Time'], df_base['CWND'] / MSS, 
         label=f'Baseline (Node {TARGET_NODE})', 
         color='blue', linewidth=1.5, alpha=0.9,
         marker='+', markersize=4, markeredgewidth=0.5)
# 2. Plot Fibre Cut (Red Line)
plt.plot(df_cut['Time'], df_cut['CWND'] / MSS, 
         label=f'Fibre Cut (Node {TARGET_NODE})', 
         color='red', linewidth=1.5, alpha=0.9,
         marker='+', markersize=4, markeredgewidth=0.5)
# 3. Add Cut Event Line
plt.axvline(x=12.0, color='black', linestyle='--', linewidth=2, label='Cut Event (t=12s)')
# Formatting
plt.title(f'Congestion Window vs Time graph for Node ID {TARGET_NODE})')
plt.xlabel('Time (s)')
plt.ylabel('Congestion Window (Packets)')
plt.legend()
plt.grid(True, linestyle='--', alpha=0.5)
# Save
plt.tight_layout()
plt.savefig('cwnd_vs_time.png', dpi=300)
plt.show()