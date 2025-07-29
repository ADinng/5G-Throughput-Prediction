import os
import re
import numpy as np
import subprocess
from collections import defaultdict
from collections import OrderedDict
from argparse import ArgumentParser
import csv


'''Script for combining UE performance metrics

    Created by Darijo Raca, MISL, Computer Science, UCC, 14.10.2016.
    Updated by Xingmei Ding, Computer Science, UCC, 12.06.2025.

    Tasks:
        - take folder as a input
        - find all files belonging to one user
        - combine metrices and save them to output folder
'''

parser = ArgumentParser(description="Combining the results for web client's performance metrics")


parser.add_argument('--path', '-p',
                    dest="dir_path",
                    action="store",
                    help="Directory where traces should be",
                    required=True)

parser.add_argument('--opath', '-op',
                    dest="output_path",
                    action="store",
                    help="Directory where results should be saved",
                    required=True)

parser.add_argument('--numUE', '-nu',
                    dest="num_ue",
                    action="store",
                    help="Number of UE",
                    default=210)

parser.add_argument('--interval', '-il',
                    dest="interval",
                    action="store",
                    help="Window size (bins)",
                    default=4.0)

parser.add_argument('--traceDur', '-td',
                    dest="trace_duration",
                    action="store",
                    help="Trace Duration (seconds)",
                    default=490)

parser.add_argument('--do_not_include', '-di',
                    dest="not_include",
                    action="store",
                    help="List of elements that need not to be included (separated by comma (',')",
                    default=None)


# Expt parameters
args = parser.parse_args()

def save_metric_to_file(_file_name,values):

   
    #print values[0]			
    # create a output folder if it is not already created
    if not os.path.isdir(args.output_path):
        os.system('mkdir -p %s'%args.output_path)
 
    f_out = None
    h2 = list(values["4.0"].keys())
    if args.not_include != None:
        array__ = args.not_include.split(',')
        #print array__
        for aa in array__:
            if aa in h2:
                # print(h2)
                h2.remove(aa)
    #print h2
    header = ["Time"] + h2
    full_path_name = os.path.join(args.output_path, _file_name) 
    if not os.path.isfile(full_path_name):
        f_out = open(full_path_name,'w') 
        spamwriter = csv.writer(f_out, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
        
        spamwriter.writerow(header)    
    else:
        f_out = open(full_path_name,'a') 
    start = args.interval
    while float(start)<=int(args.trace_duration):
        value = []
        value.append(start)
        for head in header:
            if head != "Time":
                
                if head in values[start]:
                    value.append(values[start][head])
                else:
                    #print start
                    value.append(values[str(int(float(start)))][head])
                
        #print start
        spamwriter = csv.writer(f_out, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
        spamwriter.writerow(value)
        start = str(float(start) + float(args.interval)) 
    f_out.close()   

def combine_values(list_of_names,dirname,ue_prefix):
    
    f_handler = []
    l_dict = []
    final_dict = defaultdict(lambda : defaultdict(float))
    for name in list_of_names:
        #print name
        f_handler.append(open(os.path.join(dirname,name),'r'))
    
    for f in f_handler:
        l_dict.append(csv.DictReader(f))
    
    for dr in l_dict:
        time = "0.25"
        
        for row in dr:
            #print row["Time"]
            if float(row["Time"]) >= 0.0 and float(row["Time"]) <= int(args.trace_duration):
                
                for keys in row:
                    #print row["Time"] + " == " +  time
                    if keys != "Time":
                        #print keys
                        #print row
                        final_dict[row["Time"]][keys] = float(row[keys])
                
            #print time
    for f in f_handler:
        f.close()
    #print final_dict    
    save_metric_to_file(ue_prefix+"comb.csv",final_dict)

for i in range(int(args.num_ue)):
    

    ue_prefix = "UE_" + str(i+1) +"-"
    f_names = []
    #print ue_prefix
    for dirname, dirnames, filenames in os.walk(args.dir_path):
        for filename in filenames: 
            if ue_prefix in filename:
                # print (filename)
                f_names.append(filename)
        combine_values(f_names,dirname,ue_prefix)