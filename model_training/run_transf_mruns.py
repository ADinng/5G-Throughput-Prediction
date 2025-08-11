import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--folder_path", help="Path to folder with files")
parser.add_argument("--output_path", help="Path where to save transformed files")
parser.add_argument("--active_only", type=str, default="False", help="Whether to use only active users")
parser.add_argument("--cmps_mode", type=str, default="100",
                    help="100: runs=[1], cmps=[100]; 1050: runs=[1..10], cmps=[10,50]")


parser.add_argument("--history", type=int, default=5, help="History length")
parser.add_argument("--horizon", type=int, default=5, help="Horizon length")

# runs = [1]
# cmps = [100]

# runs = [1,2,3,4,5,6,7,8,9,10]
# cmps = [10, 50]

# history = 5
# horizon = 5

# history = 5
# horizon = 2

if __name__ == "__main__":
  args = parser.parse_args()
  
  history = args.history
  horizon = args.horizon

  if args.cmps_mode == "100":
    runs = [1]
    cmps = [100]
  elif args.cmps_mode == "1050":
    runs = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    cmps = [10, 50]
  else:
    raise ValueError("cmps_mode must be '100' or '1050' (cooperation ratio only supports 10%, 50%, 100%)")


  # python transform_dataset_mthread.py --folder_path ../col_training_new_tcp/Run5/CMP1 --output_path ../final_dataset_tcp_H1H1/Run5/CMP0 --target_metric THR --history 1 --horizon 1
  for run in runs:
    for cmp_ in cmps:
      input_folder = os.path.join(args.folder_path, "CMP%d"%cmp_+ "/Run_%d"%run )
      name = args.output_path.strip("/") + "_H%dH%d"%(history, horizon)
      final_out_name = os.path.join(name, "Run_%d"%run + "/CMP%d"%cmp_)
      print(input_folder)
      # print(final_out_name)
      # os.system("python transform_dataset_mthread_one_cell.py --active_only False --folder_path %s --output_path %s --target_metric THR --history %d --horizon %d"%(input_folder, final_out_name, history, horizon))
      os.system("python model_training/transform_dataset_mthread_one_cell.py --active_only %s --folder_path %s --output_path %s --target_metric THR --history %d --horizon %d"%(args.active_only, input_folder, final_out_name, history, horizon))
      # python transform_dataset_mthread_one_cell.py --folder_path ../one_cell_U45_AR_1000s/filtered/ --output_path ../final_dataset_U45_AR_1000s/CMP0/ --target_metric THR --history 5 --horizon 5

