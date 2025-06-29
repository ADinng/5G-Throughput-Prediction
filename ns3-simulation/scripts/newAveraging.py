from collections import defaultdict
import os
import numpy as np
from argparse import ArgumentParser
import pandas as pd
import re
from math import log10
'''Script for averaging UE performance metrics - new version (re-written)

    Created by Darijo Raca, MISL, Computer Science, UCC, 02.02.2017.

    Prerequisite:
        - script expects to have a column with name "Time" which will be used for creating bins 
          for all other columns. User can specify other column
        - give a fodler path where raw files are stored or give a raw file
        - give a output folder (new file will have a same name with added suffix)
        - give interval for binning (default value will be 1 second)
        - how to summarize bins (mean, median, percentile, sum)
        - how to deal with missing values (when interval is empty): give a value. 
          Default is 'NaN'
    Tasks:
        - load a csv file
        - take last value from 'Time' column (or specified) and generate dictonary with all the intervals
          initialize all intervals with missing value replacement
        - go thorugh rows of csv file and put values to appropriate interval
        - at the end apply summarize method
        - save the file
'''
parser = ArgumentParser(description="Averaging the results for web client's performance metrics")

parser.add_argument('-target_column', '-tc',
                    dest="target_column",
                    action="store",
                    help="Target column used for creating bins",
                    default="Time")


parser.add_argument('--path', '-p',
                    dest="dir_path",
                    action="store",
                    help="Folder with the traces (or file)",
                    required=True)

parser.add_argument('--opath', '-op',
                    dest="output_path",
                    action="store",
                    help="Folder where results should be saved",
                    required=True)

parser.add_argument('--interval', '-il',
                    dest="interval",
                    action="store",
                    help="Time interval for averaging",
                    default=1.0)

parser.add_argument('--mode', '-m',
                    dest="mode",
                    action="store",
                    help="Method for summarizing intervals",
                    default="mean")

parser.add_argument('--missing_value', '-mv',
                    dest="missing_value",
                    action="store",
                    help="Value which will be put if the interval is missing samples. By default NaN value is used, but it can be anything",
                    default="NaN")

# Expt parameters
args = parser.parse_args()

REG_UE = r'UE_([0-9]+)-.*.csv'
#REG_UE = r'(.*).csv'

for dirname, dirnames, filenames in os.walk(args.dir_path):
    # we are assuming that all the files are having UE prefix

    for filename in filenames:
      content = re.search(REG_UE,filename)
      numUE = 1
      content = 1
      if content:
        #numUE = int(content.group(1))%2
        if ".csv" in filename and numUE!=0:

            # print (filename)
            fullname = os.path.join(dirname,filename)
            # open a csv file using pandas DataFrame
            #fullname= '/home/darijo/Documents/ATT/24.10/TimeStamped/cqi1_timestamps_nd.csv'
            pdf = pd.read_csv(fullname)
            tt = pdf.map(type).eq(str).any()
    
            if pdf.empty or sum(tt) > 0:
                continue
            ##### INTERVALS CREATION ###########
            window_interval = np.around(float(args.interval),decimals=2)
            end_time = np.around(float(pdf['Time'].iloc[-1]),decimals=2) + 2*window_interval
            #print end_time
            window_array = np.arange(window_interval,end_time,window_interval)
            #print 
            window_values = {}
            ####################################


            ##### INITIAL EMPTY DICT CREATION ###########
            # get column names - if we have multiple features
            col_names = list(pdf)
            # remove target metric - 'Time' by default. We will add our own timeline
            if args.target_column in col_names:
                col_names.remove(args.target_column)

            # create dictonaries and initiliaze them with missing value
            for col_name in col_names:
                if col_name != args.target_column:
                    window_values[col_name] = {}
                    for w_int in window_array:
                        #print w_int
                        window_values[col_name][w_int] = args.missing_value
            ####################################

            #### READ CSV AND STORE VALUES IN APPROPRIATE BINS #
            # read row by row from csv. put appropriate values to the 
            for row in pdf.iterrows():

    
                curr_time = row[1][args.target_column]
                #print curr_time
                index = int(curr_time/window_interval) * window_interval + window_interval
                for col_name in col_names:
                    if window_values[col_name][index] == args.missing_value:
                        window_values[col_name][index] = []
                        window_values[col_name][index].append(row[1][col_name])
                    else:
                        window_values[col_name][index].append(row[1][col_name])
            ###########################################################            


            ######### SUMMARIZE INTERVALS #########

            new_dict = defaultdict(list)

            if args.mode == 'mean':
                for w_int in window_array:
                    new_dict[args.target_column].append(w_int)
                    for name in window_values:
                        
                        if window_values[name][w_int] != args.missing_value:
                            #if name == 'RSRP':
                            #    new_dict[name].append(np.around(10*log10(np.mean(window_values[name][w_int])),decimals=2))
                            #else:
                                new_dict[name].append(np.around(np.mean(window_values[name][w_int]),decimals=2))
                        else:
                            new_dict[name].append(window_values[name][w_int])
            elif args.mode == 'inst':
                
                for w_int in window_array:
                    new_dict[args.target_column].append(w_int)
                    for name in window_values:
                        
                        if window_values[name][w_int] != args.missing_value:
                            if name == 'THR':
                               new_dict[name].append(np.around(np.mean(window_values[name][w_int]),decimals=2))
                            else:
                                new_dict[name].append(np.around(window_values[name][w_int][-1],decimals=2))
                        else:
                            new_dict[name].append(window_values[name][w_int])
             

            #######################################
            if not os.path.isdir(args.output_path):
                os.system('mkdir -p %s'%args.output_path)
            new_filename = filename.split('.')[0]
            new_filename = new_filename + '-' + args.mode + '.csv'
            full_new_filename = os.path.join(args.output_path,new_filename)
            pd_out = pd.DataFrame(new_dict)
            pd_out.to_csv(full_new_filename,index=False)
   
            
                        

