import pandas as pd
import matplotlib.pyplot as plt
import os

file_baseline = "tcp-hop-count.csv"
file_cut = "CUT-tcp-hop-count.csv"
cut_time = 12.0

def plot_clean_hops(file_base, file_cut):
    plt.figure(figsize=(10, 6))

    # --- Baseline (normal operation) ---
    if os.path.exists(file_base):
        df_base = pd.read_csv(file_base)               # USE HEADER FROM FILE
        df_base["Time"] = pd.to_numeric(df_base["Time"], errors="coerce")
        df_base["Hops"] = pd.to_numeric(df_base["Hops"], errors="coerce")
        df_base = df_base.dropna().sort_values("Time")

        # Downsample to 0.1s bins
        df_base_ds = df_base.groupby(df_base["Time"].round(1)).mean()

        plt.plot(df_base_ds.index,
                 df_base_ds["Hops"].rolling(5).mean(),
                 label="Normal Operation (Avg Hop Count)",
                 color="blue", linestyle="--", linewidth=2)

    # --- Fibre Cut Scenario ---
    if os.path.exists(file_cut):
        df_cut = pd.read_csv(file_cut)                # USE HEADER FROM FILE
        df_cut["Time"] = pd.to_numeric(df_cut["Time"], errors="coerce")
        df_cut["Hops"] = pd.to_numeric(df_cut["Hops"], errors="coerce")
        df_cut = df_cut.dropna().sort_values("Time")

        df_cut_ds = df_cut.groupby(df_cut["Time"].round(1)).mean()

        plt.plot(df_cut_ds.index,
                 df_cut_ds["Hops"].rolling(5).mean(),
                 label="Fibre Cut Scenario (Avg Hop Count)",
                 color="red", linewidth=2)

        # Cut event line
        plt.axvline(x=cut_time, color='black', linestyle=':', linewidth=2)
        plt.text(cut_time + 0.1,
                 df_cut_ds["Hops"].max() * 0.9,
                 "Fibre Cut (12s)",
                 fontsize=9)

    # --- Formatting ---
    plt.title("Hop Count Evolution Before and After Fibre Cut", fontsize=14)
    plt.xlabel("Time (seconds)")
    plt.ylabel("Average Hop Count")
    plt.grid(True, linestyle="--", alpha=0.4)
    plt.legend()
    plt.tight_layout()
    plt.show()


plot_clean_hops(file_baseline, file_cut)
