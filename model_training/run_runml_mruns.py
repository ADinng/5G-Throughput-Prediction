import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--folder_path", help="Path to folder with files")
parser.add_argument("--output_path", help="Path where to save transformed files")
parser.add_argument("--cmps_mode", type=str, default="100",
                    help="100: runs=[1], cmps=[100]; 1050: runs=[1..10], cmps=[10,50]")

# runs = [1,2,3,4,5]

# cmps = [10, 50]

# runs = [1]
# cmps = [100]

# runs = [1,2,3,4,5,6,7,8,9,10]
# cmps = [10, 50]


history = 5
horizon = 5


if __name__ == "__main__":
  args = parser.parse_args()

  if args.cmps_mode == "100":
    runs = [1]
    cmps = [100]
  elif args.cmps_mode == "1050":
    runs = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    cmps = [10, 50]
  else:
    raise ValueError("cmps_mode must be '100' or '1050' (cooperation ratio only supports 10%, 50%, 100%)")

  # python run_ml.py --folder_path ../final_dataset_tcp_H1H1/Run1/CMP0/ --output_path ../final_dataset_new_res_H1H1/Run1/CMP0/ --target_metric THR --type raw --scen_name CMP0 --gran 1s
  for run in runs:
    for cmp_ in cmps:
      input_folder = os.path.join(args.folder_path, "Run_%d"%run + "/CMP%d"%cmp_)
      name = args.output_path.strip("/") + "_H%dH%d"%(history, horizon)
      final_out_name = os.path.join(name, "Run_%d"%run + "/CMP%d"%cmp_)
      os.system("python model_training/run_ml.py --remove_features PDCP_Throughput --folder_path %s --output_path %s --target_metric THR --type raw --scen_name CMP%d --gran 1s"%(input_folder, final_out_name, cmp_))

# 
