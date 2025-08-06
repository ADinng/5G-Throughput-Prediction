import os
import pandas as pd
import matplotlib.pyplot as plt
from PIL import Image
import argparse
import numpy as np

# 1. about select best runs

    # For each of the 10 runs, we compute the median ARE (Absolute Relative Error)
    # from the CMP10 and CMP50 settings. Instead of picking the run with the lowest median,
    # we select the one whose median error is closest to the average of all medians.
    # This helps identify the most representative run, avoiding bias from exceptionally good or bad runs.


parser = argparse.ArgumentParser()
parser.add_argument("--folder_path", type=str, required=True, help="Base directory path for results")

def select_best_runs(base_path):
    cmp10_medians = {}
    cmp50_medians = {}
    
    for run in [f"Run_{i}" for i in range(1, 11)]:
        cmp10_csv = os.path.join(base_path, run, "CMP10", f"UEs_H5F5_1s_group_raw_CMP10.csv")
        cmp50_csv = os.path.join(base_path, run, "CMP50", f"UEs_H5F5_1s_group_raw_CMP50.csv")
        
        if os.path.isfile(cmp10_csv):
            df10 = pd.read_csv(cmp10_csv)
            cmp10_medians[run] = df10["error"].median()
        if os.path.isfile(cmp50_csv):
            df50 = pd.read_csv(cmp50_csv)
            cmp50_medians[run] = df50["error"].median()
    
    # lowest median
    # best_cmp10_run = min(cmp10_medians, key=cmp10_medians.get) if cmp10_medians else None
    # best_cmp50_run = min(cmp50_medians, key=cmp50_medians.get) if cmp50_medians else None

    # closest to the average of all medians
    mean_median10 = np.mean(list(cmp10_medians.values()))
    best_cmp10_run = min(cmp10_medians, key=lambda r: abs(cmp10_medians[r] - mean_median10))

    mean_median50 = np.mean(list(cmp50_medians.values()))
    best_cmp50_run = min(cmp50_medians, key=lambda r: abs(cmp50_medians[r] - mean_median50))
    return best_cmp10_run, best_cmp50_run

def load_image(image_path):
    try:
        return Image.open(image_path)
    except FileNotFoundError:
        print(f"Image not found: {image_path}")
        return None

def load_error_data(base_path, run_name, cmp_label):
    csv_path = os.path.join(base_path, run_name, cmp_label, f"UEs_H5F5_1s_group_raw_{cmp_label}.csv")
    if os.path.isfile(csv_path):
        df = pd.read_csv(csv_path)
        return df["error"].values
    else:
        print(f"File not found: {csv_path}")
        return None

def plot_combined_boxplot(data_list, labels, save_path):

    plt.figure(figsize=(8,6))
    plt.boxplot(data_list, showfliers=False, whis=[5, 90])
    plt.xticks(range(1, len(labels)+1), labels)
    plt.ylabel("ARE (%)")
    plt.title("Comparison of Prediction Errors (ARE) across Cooperation Levels")
    plt.grid(axis='y', linestyle='--', alpha=0.7)

    plt.savefig(save_path, bbox_inches='tight', format="png", dpi=250)
    # plt.show()

if __name__ == "__main__":
    args = parser.parse_args()
    # base_path = "./results_ml_off_network_H5H5_ue20_1000s"
    base_path = args.folder_path
    if not os.path.isdir(base_path):
        print(f"Error: directory {base_path} does not exist!")
        exit(1)

    print("Base path: ", base_path)
    best_cmp10, best_cmp50 = select_best_runs(base_path)
    print(f"Best CMP10 run: {best_cmp10}")
    print(f"Best CMP50 run: {best_cmp50}")

    cmp100_run = "Run_1"
    
    data_10 = load_error_data(base_path, best_cmp10, "CMP10")
    data_50 = load_error_data(base_path, best_cmp50, "CMP50")
    data_100 = load_error_data(base_path, cmp100_run, "CMP100")

    data_list = [data for data in [data_10, data_50, data_100] if data is not None]
    labels = ["10%", "50%", "100%"]
    
    save_path = os.path.join(base_path, "combine_boxplot.png")
    plot_combined_boxplot(data_list, labels, save_path)
