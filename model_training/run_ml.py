import argparse
from collections import defaultdict
import multiprocessing as MP
from functools import partial
from sklearn.ensemble import ExtraTreesRegressor
from sklearn.model_selection import train_test_split
import pandas as pd
import numpy as np
import os
import re
import random
import matplotlib.pyplot as plt
# regex for the trace logs UE_1-comb_N5_R1_H5F5_cleaned
REG_MEASUREMENT_TRACE = r"(UE[0-9]+).*_H([0-9]+)F([0-9]+)_cleaned.*\.csv" # UE1_C1_N1_S2_P1_final_CMP1_H5F5_cleaned
REG_MEASUREMENT_TRACE = r"(UE_[0-9]+).*_H([0-9]+)F([0-9]+)_cleaned.*\.csv" # UE_1_C1_N1_S2_P1_final_CMP1_H5F5_cleaned



# TODO: For some reason adding new argument does not work. Currently suffix for different granularity needs to be added by hand
# TODO: This needs fixing
parser = argparse.ArgumentParser()
parser.add_argument("--folder_path", help="Path to folder with traces")
parser.add_argument("--output_path", help="Path where to save cleaned traces")
parser.add_argument("--target_metric", help="Target (label) for prediction")
parser.add_argument("--remove_features", nargs="+", default=[], help="List of features used for training")
parser.add_argument("--type", help="Type of dataset used for training")
parser.add_argument("--scen_name", default="all_features", help="Scenario name in regard to features used")
parser.add_argument("--gran", help="Data granularity")


def remove_features(list_features, hist, horz):
    features_to_remove = []
    for feat in list_features:
      for i in range(hist):
        features_to_remove.append(feat+"-"+str(i))
    return features_to_remove


def list_traces(folder_path, rx):
    directory = os.fsencode(folder_path)
    list_files = []
    history_ = 0
    horizon_ = 0
    for _file in os.listdir(directory):
         filename = os.fsdecode(_file)
         print(filename, rx)
         if re.search(rx, filename):
             
             match = re.search(rx, filename)
             history_ = int(match.group(2))
             horizon_ = int(match.group(3))
             list_files.append(os.path.join(folder_path, filename))
    return list_files, history_, horizon_

def run_ml_model(trace, feat_remove):
    
    reg2 = ExtraTreesRegressor(random_state=1)
    r_pdf = pd.read_csv(trace)
    # drop Drop column
    r_pdf.drop(['Drop'], axis=1, inplace=True)
    #print(feat_remove)
    if len(feat_remove) > 0:
      # let's drop all with pattern
      #print(args.remove_features)
      #r_pdf = r_pdf[r_pdf.columns.drop(list(r_pdf.filter(regex=args.remove_features)))]
      r_pdf.drop(feat_remove, axis="columns", inplace=True)
    org_cols = r_pdf.columns
    
    y = r_pdf[str(args.target_metric)]
    X = r_pdf.drop(args.target_metric, axis="columns")
    
    X_train, X_val, y_train, y_val = train_test_split(X, y, train_size=0.8, random_state=4, shuffle=False)

    reg2.fit(X_train, y_train)
    estimate = reg2.predict(X_val)

    # save results
    _dict = defaultdict(list)
    _dict["True"] = list(y_val)
    _dict["Predicted"] = list(estimate.flatten() )
    return _dict

def run_ml_model_all_users(train_d, val_d):
    reg2 = ExtraTreesRegressor(random_state=1, n_jobs=-1)

    y_train = train_d[args.target_metric]
    X_train = train_d.drop(args.target_metric, axis="columns")

    y_val = val_d[args.target_metric]
    X_val = val_d.drop(args.target_metric, axis="columns")
    
    reg2.fit(X_train, y_train)
    estimate = reg2.predict(X_val)

    # save results
    _dict = defaultdict(list)
    _dict["True"] = list(y_val)
    _dict["Predicted"] = list(estimate.flatten() )

    X_val["True"] = list(y_val)
    X_val["Predicted"] = list(estimate.flatten() )

    return X_val
    
def merge_logs(traces, feat_remove):
  _pdf = pd.read_csv(traces[0])
  # drop last 500
  #_pdf.drop(_pdf.tail(500).index,inplace = True)
  for _trace in traces[1:]:
    # drop 500
    p = pd.read_csv(_trace)
    #p.drop(p.tail(500).index,inplace = True)
    #_pdf = _pdf.append(p, ignore_index=True)
    _pdf = pd.concat([_pdf, p], ignore_index=True)
    ###
    #_pdf = _pdf.append(pd.read_csv(_trace), ignore_index=True)
  if len(feat_remove) > 0:
      # let's drop all with pattern
    
      _pdf = _pdf[_pdf.columns.drop(list(_pdf.filter(regex=args.remove_features[0])))]
      #_pdf.drop(feat_remove, axis="columns", inplace=True)
  
  #_pdf = _pdf.drop_duplicates(subset='THR', keep="first")
  return _pdf

def simple_boxplot(data_opr, **params):
  
  fig, ax = plt.subplots()
  bp = ax.boxplot(data_opr, showfliers=False, whis=[5,90])
  ax.set_xticklabels([params["x_title"]])
  plt.ylabel(params["y_title"])
  #plt.show()
  fig.savefig(params["path"], orientation='landscape', bbox_inches='tight', format="png", dpi=250)

def simple_timeseries(true, val, **params):
  
  fig, ax = plt.subplots()
  ax.plot(true, label="True")
  ax.plot(val, label="Validation")
  plt.xlabel(params["x_title"])
  plt.ylabel(params["y_title"])
  plt.legend()
  fig.savefig(params["path"], orientation='landscape', bbox_inches='tight', format="png", dpi=250)

if __name__ == "__main__" :
  args = parser.parse_args()
  print (args.folder_path)
  meas_traces, _history, _horizon = list_traces(args.folder_path, REG_MEASUREMENT_TRACE)
  feats_to_remove = remove_features(args.remove_features, _history, _horizon)
 
  '''
  with MP.Pool(MP.cpu_count()) as p:
        all_results = p.map(partial(run_ml_model, feat_remove=feats_to_remove), meas_traces)
  pdf_error = pd.DataFrame(all_results[0])
  for _trace in all_results[1:]:
   pdf_error = pdf_error.append(pd.DataFrame(_trace), ignore_index=True)
      
  pdf_error["error"] = 100*np.absolute(pdf_error["True"] - pdf_error["Predicted"]) / pdf_error["True"]
  os.system("mkdir -p %s"%args.output_path)
  match = re.search(REG_MEASUREMENT_TRACE, meas_traces[0].split("/")[-1])
  full_out_path = os.path.join(args.output_path, "UEs"+"_H%dF%d_%s_all_%s_%s.csv"%((_history), int(_horizon), args.gran, args.type, args.scen_name))
  pdf_error.to_csv(full_out_path, index=False)

  full_out_path_image = os.path.join(args.output_path, "UEs"+"_H%dF%d_%s_all_%s_%s.png"%((_history), int(_horizon), args.gran, args.type, args.scen_name))
  simple_boxplot(pdf_error["error"], path=full_out_path_image, x_title="Validation", y_title="ARE (%)")

  full_out_path_image = os.path.join(args.output_path, "UEs"+"_H%dF%d_%s_all_ts_%s_%s.png"%((_history), int(_horizon), args.gran, args.type, args.scen_name))
  simple_timeseries(pdf_error["True"], pdf_error["Predicted"], path=full_out_path_image, x_title="Time (s)", y_title="Throughput")
  '''
  # training on 80% of total data (multiple users) and predicting for the remaining ones
  train_size = int(len(meas_traces)*0.8)
  random.seed(4)
  random.shuffle(meas_traces)
  train_traces = meas_traces[:train_size]
  val_traces = meas_traces[train_size:]

  train_data = merge_logs(train_traces, feats_to_remove)
  val_data = merge_logs(val_traces, feats_to_remove)
  res_dict = run_ml_model_all_users(train_data, val_data)
  pdf_error = pd.DataFrame(res_dict)
  pdf_error["error"] = 100*np.absolute(pdf_error["True"] - pdf_error["Predicted"]) / pdf_error["True"]
  os.system("mkdir -p %s"%args.output_path)
  match = re.search(REG_MEASUREMENT_TRACE, meas_traces[0].split("/")[-1])
  full_out_path = os.path.join(args.output_path, "UEs"+"_H%dF%d_%s_group_%s_%s.csv"%((_history), int(_horizon), args.gran, args.type, args.scen_name))
  pdf_error.to_csv(full_out_path, index=False)

  # plotting
  full_out_path_image = os.path.join(args.output_path, "UEs"+"_H%dF%d_%s_group_%s_%s.png"%((_history), int(_horizon), args.gran, args.type, args.scen_name))
  simple_boxplot(pdf_error["error"], path=full_out_path_image, x_title="Validation", y_title="ARE (%)")

  full_out_path_image = os.path.join(args.output_path, "UEs"+"_H%dF%d_%s_group_ts_%s_%s.png"%((_history), int(_horizon), args.gran, args.type, args.scen_name))
  simple_timeseries(pdf_error["True"], pdf_error["Predicted"], path=full_out_path_image, x_title="Time (s)", y_title="Throughput")
