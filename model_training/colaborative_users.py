import argparse
import os
import re
import random 
import pandas as pd
import numpy as np



parser = argparse.ArgumentParser()
parser.add_argument("--folder_path", help="Path to folder with traces")
parser.add_argument("--output_path", help="Path where to save cleaned traces")
parser.add_argument("--percent_comp_users", help="percent of colaborative users")
parser.add_argument("--num_runs", help="Number of runs")

REG_MEASUREMENT_TRACE = r".*\.csv"
REG_FILTER_USER = r"UE_([0-9]+)-comb\.csv" # UE_1-comb

def list_traces(folder_path, rx):
    directory = os.fsencode(folder_path)
    list_files = []
    for _file in os.listdir(directory):
         filename = os.fsdecode(_file)
         
         if re.search(rx, filename):
             
             match = re.search(rx, filename)

             list_files.append(os.path.join(folder_path, filename))
    return list_files

def prepare_pdf_comp_users(path_list, comp_u_ids, start_time = 0):
    pdfs_list = []
    for path_ in path_list:
         match = int(re.search(REG_FILTER_USER, path_).group(1) )
         if match in comp_u_ids:
             pdf = pd.read_csv(path_)
             m_pdf = pdf[pdf['Time'] >= start_time]
             pdfs_list.append(m_pdf)
    
    return pdfs_list

def calc_stats(pdf_, list_pdfs):
        
    # feat_list = ["CQI", "PDCP_Throughput", "THR", "Delay", "RSRP", "SINR"]
    feat_list = ["CQI", "THR", "RSRP", "SINR"]
    
    for feature in feat_list:
        for stat in ["min", "max", "mean", "25%", "50%", "75%", "std"]:
            pdf_[f"{feature}_{stat}"] = np.nan  # Predefine columns with NaN
    
    for index, row in pdf_.iterrows():
        df = pd.DataFrame(columns=feat_list)
        curr_time = row["Time"]
        for colab_pdf in list_pdfs:
           sliced_pdf = colab_pdf[colab_pdf["Time"] == curr_time]
          
           if not sliced_pdf["Tx"].isnull().any():
              
               df = pd.concat([df, sliced_pdf[feat_list]], ignore_index=True)
        
        
        #print (df)
        # add user statistic
        #df = pd.concat([df, row[feat_list]], ignore_index=True)
        #print ("After adding user")
        # line below for adding target user as well
        # df.loc[len(df)] = row[feat_list]
        #print (df)
     
        df["THR"] = pd.to_numeric(df["THR"], errors="coerce")
        res = df.describe(percentiles=[0.25, 0.5, 0.75])
        
        
       # try:
        if not df.empty:
           #print("Number of active users: ", len(df), " out of ", len(list_pdfs))
           pdf_.loc[index, "Active_Users"] = len(df)
           for feature in feat_list:        
                
                    if feature in res:
                        pdf_.loc[index, feature+ "_min"] = res[feature].loc["min"]
                        pdf_.loc[index, feature+ "_max"] = res[feature].loc["max"]
                        pdf_.loc[index, feature+ "_mean"] = res[feature].loc["mean"]
                        pdf_.loc[index, feature+ "_25%"] = res[feature].loc["25%"]
                        pdf_.loc[index, feature+ "_50%"] = res[feature].loc["50%"]
                        pdf_.loc[index, feature+ "_75%"] = res[feature].loc["75%"]
                        pdf_.loc[index, feature+ "_std"] = res[feature].loc["std"]
                    else:
                        print(df.dtypes)
                        print(df)
                        print(df.describe( include="all"))
                        if "THR" in df.columns:
                            print("THR column is present in df.")
                        else:
                            print("THR column is missing from df.")
                        
                
        #except:
        #    print("Error")
            #print(res)
            #print(df)
            

    # pdf_["Active_Users"].fillna(0, inplace=True) 
    pdf_["Active_Users"] = pdf_["Active_Users"].fillna(0)
    return pdf_

if __name__ == "__main__" :
  args = parser.parse_args()
  
  meas_traces = list_traces(args.folder_path, REG_MEASUREMENT_TRACE)
  total_num_users = len(meas_traces)
  
  for run_ in range(int(args.num_runs)):
    random.seed(int(run_))
    for meas_trace in meas_traces:
      

      list_users_id = list(range(1,total_num_users+1) )
      match = re.search(REG_FILTER_USER, meas_trace)
      curr_user = int(match.group(1) )
     
      # remove current user from the list
      list_users_id.remove(curr_user)
      # get competing users id    
      num_competing = ((total_num_users-1) * int(args.percent_comp_users))//100
      colaborative_users_id = random.sample(list_users_id, k=num_competing)
      print("competing users for user %d are"%curr_user, colaborative_users_id)
      
      comp_pdfs_list = prepare_pdf_comp_users(meas_traces, colaborative_users_id)
      curr_pdf = pd.read_csv(meas_trace)
      # get stats
      modified_pdf = calc_stats(curr_pdf, comp_pdfs_list)
      
      filename_f = meas_trace.split("/")[-1].split(".")[0] + "_C" + str(args.percent_comp_users) + "_R"+str(run_+1)+"_UE"+str(total_num_users)+"_P0_final.csv"
      print(filename_f)
    
      dir_path = os.path.join(args.output_path, "CMP" + str(args.percent_comp_users), "Run_%d"%(run_+1))
      os.system("mkdir -p %s"%dir_path)
      modified_pdf.to_csv(os.path.join(dir_path, filename_f), index = False)
      
      
  
