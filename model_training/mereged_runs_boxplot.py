import os
import pandas as pd
import matplotlib.pyplot as plt
import argparse
import numpy as np


parser = argparse.ArgumentParser()
parser.add_argument("--folder_path", type=str, required=True, help="Base directory path for results")


def collect_errors_all_runs(base_path, cmp_label):
    all_errors = []
    if cmp_label == "CMP100":
        runs = ["Run_1"]   
    else:
        runs = [f"Run_{i}" for i in range(1, 11)]  

    for run in runs:
        csv_path = os.path.join(base_path, run, cmp_label, f"UEs_H5F5_1s_group_raw_{cmp_label}.csv")
        if os.path.isfile(csv_path):
            df = pd.read_csv(csv_path)
            all_errors.extend(df["error"].values)
        else:
            print(f"[Warning] Missing: {csv_path}")
    return np.array(all_errors)


# --- plot boxplot ---
def plot_combined_boxplot(data_list, labels, save_path):
    plt.figure(figsize=(8, 6))
    plt.boxplot(data_list, showfliers=False, whis=[5, 90])
    plt.xticks(range(1, len(labels) + 1), labels)
    plt.ylabel("ARE (%)")
    plt.title("Merged Error Distribution across Cooperation Levels (10 Runs)")
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.savefig(save_path, bbox_inches='tight', format="png", dpi=250)
    print(f"[Saved] Boxplot saved to: {save_path}")

# --- main process ---
if __name__ == "__main__":
    args = parser.parse_args()
    base_path = args.folder_path
    if not os.path.isdir(base_path):
        print(f"[Error] Directory not found: {base_path}")
        exit(1)

    print("[Info] Base path:", base_path)

    data_10 = collect_errors_all_runs(base_path, "CMP10")
    data_50 = collect_errors_all_runs(base_path, "CMP50")
    data_100 = collect_errors_all_runs(base_path, "CMP100")
    

    # clean empty data
    data_list = [data for data in [data_10, data_50, data_100] if len(data) > 0]
    labels = ["10%", "50%", "100%"][:len(data_list)]

    if len(data_list) == 0:
        print("[Error] No data found for any cooperation level.")
        exit(1)

    save_path = os.path.join(base_path, "merged_10runs_boxplot.png")
    plot_combined_boxplot(data_list, labels, save_path)
