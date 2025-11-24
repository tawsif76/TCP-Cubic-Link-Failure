import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

file_baseline = "tcp-all-delays.csv"
file_cut      = "CUT-tcp-all-delays.csv"
cut_time = 12.0

def plot_delay_timeseries(file_base, file_cut):

    plt.figure(figsize=(12, 5))

    # --- Baseline ---
    df_base = pd.read_csv(file_base)
    df_base["Time"]  = pd.to_numeric(df_base["Time"],  errors="coerce")
    df_base["Delay"] = pd.to_numeric(df_base["Delay"], errors="coerce")
    df_base = df_base.dropna().sort_values("Time")

    # Smooth curve (rolling mean)
    df_base["Smooth"] = df_base["Delay"].rolling(window=200).mean()

    plt.plot(df_base["Time"], df_base["Smooth"],
             label="Baseline TCP (No Fibre Cut)",
             color="blue", linewidth=2, alpha=0.9)


    # --- CUT Scenario ---
    df_cut = pd.read_csv(file_cut)
    df_cut["Time"]  = pd.to_numeric(df_cut["Time"],  errors="coerce")
    df_cut["Delay"] = pd.to_numeric(df_cut["Delay"], errors="coerce")
    df_cut = df_cut.dropna().sort_values("Time")

    df_cut["Smooth"] = df_cut["Delay"].rolling(window=200).mean()

    plt.plot(df_cut["Time"], df_cut["Smooth"],
             label="TCP After Fibre Cut",
             color="red", linewidth=2, alpha=0.9)

    # --- Fibre Cut Marker ---
    plt.axvline(x=cut_time, linestyle=":", color="black", linewidth=2)
    plt.text(cut_time + 0.2,
             df_cut["Smooth"].max() * 0.9,
             "Fibre Cut", fontsize=10)

    # --- Labels & Style ---
    plt.title("TCP Packet Delay Over Time (Before and After Fibre Cut)", fontsize=14)
    plt.xlabel("Time (seconds)")
    plt.ylabel("Delay (seconds)")
    plt.grid(True, linestyle="--", alpha=0.4)
    plt.legend()
    plt.tight_layout()
    plt.show()


plot_delay_timeseries(file_baseline, file_cut)
