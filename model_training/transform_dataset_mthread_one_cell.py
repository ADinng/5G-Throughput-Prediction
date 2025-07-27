import pandas as pd
from os import walk
import os
import re
import argparse
import numpy as np
import multiprocessing as MP
from functools import partial


# regex for the trace logs
REG_MEASUREMENT_TRACE = r"\w+_P[0-9]+_final\.csv"   # UE1_C1_N1_S0_P0_final 
# REG_MEASUREMENT_TRACE = r".*\.csv" 
#REG_MEASUREMENT_TRACE = "\w+.*-comb_filtered\.csv"   # UE1_C1_N1_S0_P0_final UE_1-comb_filtered_C25_R1_P0_final
# regex for the folder structure 2023-09-14-15-43-26_N5_R1
REG_MEASUREMENT_FOLDER = r"\w+/(CMP[0-9]+)/"  # ../col_training/CMP1/UE1_C4_N1_S3_P1_final.csv


# list of features used for model training
FEATURE_LIST = ["CQI", "Delay", "RSRP", "SINR", "CQI_mean", "THR_mean", "PDCP_Throughput_mean", "Delay_mean", "RSRP_mean", "SINR_mean","CQI_mean", "PDCP_Throughput_std", "THR_std", "Delay_std", "RSRP_std", "SINR_std" , "CQI_min", "THR_min", "PDCP_Throughput_min", "Delay_min", "RSRP_min", "SINR_min", "CQI_25%", "THR_25%", "PDCP_Throughput_25%", "Delay_25%", "RSRP_25%", "SINR_25%", "CQI_50%", "THR_50%", "PDCP_Throughput_50%", "Delay_50%", "RSRP_50%", "SINR_50%", "CQI_75%", "THR_75%", "PDCP_Throughput_75%", "Delay_75%", "RSRP_75%", "SINR_75%", "CQI_max", "THR_max", "PDCP_Throughput_max", "Delay_max", "RSRP_max", "SINR_max", "THR", "Active_Users", "Tx", "PDCP_Throughput"]

# list of features for base case
#FEATURE_LIST = ["CQI", "Delay", "RSRP", "SINR", "Tx", "THR"]

# for non colaborative
#FEATURE_LIST = ["CQI", "Delay", "RSRP", "SINR", "THR"]
#FEATURE_LIST = ["CCQI", "CQI", "CRSRP", "CSINR", "TBSize", "NDI", "PDCP_Throughput", "Delay", "RB", "CellID_RB", "RSRP", "RSRQ", "SINR"]
#IMP_VAL = {"SINR":-25, "RSRP":-141, "CQI":-1, "THR":-1, "Delay":-1}
IMP_VAL = {"SINR":-25, "RSRP":-141, "CQI":-1, "CQI_mean":-1, "THR_mean":-1, "PDCP_Throughput_mean":-1, "Delay_mean":-1, "RSRP_mean":-141, "SINR_mean":-25,"CQI_mean":-1, "PDCP_Throughput_std":-1, "THR_std":-1, "Delay_std":-1, "RSRP_std":-141, "SINR_std":-25 , "CQI_min":-1, "THR_min":-1, "PDCP_Throughput_min":-1, "Delay_min":-1, "RSRP_min":-141, "SINR_min":-25, "CQI_25%":-1, "PDCP_Throughput_25%":-1, "THR_25%":-1, "Delay_25%":-1, "RSRP_25%":-141, "SINR_25%":-25, "CQI_50%":-1, "THR_50%":-1, "PDCP_Throughput_50%":-1, "Delay_50%":-1, "RSRP_50%":-141, "SINR_50%":-25, "CQI_75%":-1, "THR_75%":-1, "PDCP_Throughput_75%":-1, "Delay_75%":-1, "RSRP_75%":-141, "SINR_75%":-25, "CQI_max":-1, "THR_max":-1, "PDCP_Throughput_max":-1, "Delay_max":-1, "RSRP_max":-141, "SINR_max":-25, "Tx":0, "PDCP_Throughput": -1}

#non colaborative
#IMP_VAL = {"SINR":-25, "RSRP":-141, "CQI":-1}

parser = argparse.ArgumentParser()
parser.add_argument("--folder_path", help="Path to folder with traces")
parser.add_argument("--output_path", help="Path where to save cleaned traces")
parser.add_argument("--target_metric", help="Target (label) for prediction")
parser.add_argument("--history", help="Length of history sequence")
parser.add_argument("--horizon", help="Length of averaging horizon")
parser.add_argument("--suffix", help="Optional extension to file naming")
parser.add_argument("--active_only", help="Should we keep history with full records or keep only inactive (transition from off to on)")



def list_traces(folder_path, rx):
    directory = os.fsencode(folder_path)
    list_files = []
    '''
    for (dirpath, dirnames, filenames) in walk(folder_path):
      
      if 'CMP' in dirpath:
        for filename in filenames:
          if re.search(rx, filename):
             list_files.append(os.path.join(dirpath, filename))
       '''
    for _file in os.listdir(directory):
         filename = os.fsdecode(_file)
       
         if re.search(rx, filename):
             list_files.append(os.path.join(folder_path, filename))
    
    return list_files

def load_trace(full_path, _features):
    #print (full_path)
    pdf = pd.read_csv(full_path, usecols=_features)
    return pdf

# def create_new_col_names(_history, _cols, **kwargs):
def create_new_col_names(_history, _cols, target_metric, dtype):
                  
    new_names_col = set()
    for col in _cols:
        # if col == args.target_metric and kwargs["type"] == 'object':
        if col == target_metric and dtype == 'object':
            continue
        for i in range(_history):
            new_names_col.add(col+"-"+str(i))
    # new_names_col.add(args.target_metric)
    new_names_col.add(target_metric)

    return new_names_col

# def calc_new_dataset(_history, _future, new_cols, df, trace_name):
def calc_new_dataset(_history, _future, new_cols, df, trace_name, config):
    tr_len = len(df.index)  
    # print(new_cols)
    new_df = pd.DataFrame(index=range(tr_len-(_history+_future-1)), columns = list(new_cols))
    new_df["Drop"] = 0
    for i in range(_history-1, tr_len-_future):
        for j in range(_history):
            for z in df.columns:
                # if z == args.target_metric and df[z].dtype == 'object':
                if z == config["target_metric"] and df[z].dtype == 'object':
                    continue
                
                #new_df[z+"-"+str(j)].iloc[i+1-_history] = df[z].iloc[i-j]
                new_df.loc[i+1-_history, z+"-"+str(j)] = df[z].iloc[i-j]


        # future_array = df.iloc[i+1:i+1+_future,:][args.target_metric].values
        future_array = df.iloc[i+1:i+1+_future,:][config["target_metric"]].values
        
        # logic for dropping rows if incomplete history or future
        future_activity_array = df.iloc[i+1:i+1+_future,:]["Tx"].values
        history_activity_array = df.iloc[i-_history:i,:]["Tx"].values
        # we want to drop either rows that don't have full history or otherwise
        # if args.active_only == "True":
        if config["active_only"] == "True":
           if np.sum(history_activity_array) < _history:
              new_df.loc[i+1-_history, "Drop"] = 1
        else:
           
           if np.sum(history_activity_array) > 0:
              new_df.loc[i+1-_history, "Drop"] = 1
        if np.sum(future_activity_array) < _future:
           new_df.loc[i+1-_history, "Drop"] = 1
        #print(history_activity_array, i, i-_history, np.sum(history_activity_array))
        #print(future_activity_array, i+1, i+1+_future, np.sum(future_activity_array))
        len_fut_arr = len(future_array)
        # future_array = future_array[future_array >= 0]
        if len(future_array) == 0:
            print("No future mean: " + str(np.mean(future_array)))
            #new_df[args.target_metric].iloc[i+1-_history] = np.nanmean(future_array)
            new_df.loc[i+1-_history, config["target_metric"]] = np.nanmean(future_array)
            
            
        else:
            if isinstance(future_array[0], str) and _future == 1:
                
                #new_df[args.target_metric].iloc[i+1-_history] = future_array[_future-1] 
                new_df.loc[i+1-_history, config["target_metric"]] = future_array[_future-1] 
            else:
                
                #new_df[args.target_metric].iloc[i+1-_history] = int(np.nanmean(future_array)) 
                new_df.loc[i+1-_history, config["target_metric"]] = int(np.nanmean(future_array))  
          
    # os.system("mkdir -p %s"%args.output_path)
    os.system("mkdir -p %s"%config["output_path"])
    if config["suffix"] is not None:
        full_out_path = os.path.join(config["output_path"], trace_name+"_UE%s_H%dF%d_cleaned.csv"%( config["suffix"], config["history"], config["horizon"]))
    else:
    
        full_out_path = os.path.join(config["output_path"], trace_name+"_H%dF%d_cleaned.csv"%(config["history"], config["horizon"]))
    new_df.fillna(-1,inplace=True)
    new_df = new_df.infer_objects(copy=False)
    
    if new_df[config["target_metric"]].dtype != 'object':
        new_df = new_df[new_df[config["target_metric"]] > -1]
    _cols = sorted(list(new_df.columns.values))
    _cols.pop(_cols.index(config["target_metric"]))
    _cols.append(config["target_metric"])
    new_df = new_df.reindex(_cols, axis=1)
    # drop rows where there is no activity in past and future
    new_df = new_df[new_df['Drop'] != 1]
    if  not new_df.empty:
        new_df.to_csv(full_out_path, index=False)

# def make_new_logs(trace):
def make_new_logs(trace, config):
    
    # print (trace)
    #match_ = re.search(REG_MEASUREMENT_FOLDER, trace)
    # print(match_)
    # print("------")
    r_pdf = load_trace(trace, FEATURE_LIST)
    org_cols = r_pdf.columns
    # n_cols = create_new_col_names(int(args.history), org_cols, type=r_pdf[args.target_metric].dtype)
    n_cols = create_new_col_names(config["history"], org_cols, target_metric=config["target_metric"], dtype=r_pdf[config["target_metric"]].dtype)
    fill_missing_val(r_pdf)
    
    out_trace_name = trace.split("/")[-1].split(".csv")[0]
    # calc_new_dataset(int(args.history), int(args.horizon), n_cols, r_pdf, out_trace_name)
    calc_new_dataset(config["history"], config["horizon"], n_cols, r_pdf, out_trace_name, config)
    
def fill_missing_val(df):
    for feat in IMP_VAL:
        #df[feat].fillna(IMP_VAL[feat], inplace=True)
        df[feat] = df[feat].fillna(IMP_VAL[feat])
    
    # replace Tx values with 1
    df['Tx'] = (df['Tx'] > 0).astype(int)


if __name__ == "__main__" :
    args = parser.parse_args()
    print (args.folder_path)
    meas_traces = list_traces(args.folder_path, REG_MEASUREMENT_TRACE)
    # print(meas_traces)

    config = {
        "history": int(args.history),
        "horizon": int(args.horizon),
        "target_metric": args.target_metric,
        "suffix": args.suffix,
        "output_path": args.output_path,
        "active_only": args.active_only
    }

    make_new_logs_partial = partial(make_new_logs, config=config)

    with MP.Pool(MP.cpu_count()) as p:
        print("COUNT: ", MP.cpu_count())
        # p.map(make_new_logs, meas_traces)
        p.map(make_new_logs_partial, meas_traces)
