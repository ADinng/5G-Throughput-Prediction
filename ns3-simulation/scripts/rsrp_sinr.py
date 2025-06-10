import os
import re
import numpy as np
import subprocess
from collections import defaultdict
from argparse import ArgumentParser
import csv
import pandas as pd
from math import log10
'''Script for parsing UE performance metrics (RSRP/SINR)

    Created by Darijo Raca, MISL, Computer Science, UCC, 06.10.2016.

    Tasks:
        - extract each metric to its own file and per user
        - group all metrics per user and align with timeline
        - export results to csv
        -
'''
#0.200214	1	1	0	1.6533e-14	34.8546
REG_RSRP_SINR = r'([0-9]+\.?[0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+\.?[0-9]+\w?-?[0-9]+)\s+([0-9]+\.?[0-9]*)'


parser = ArgumentParser(description="Parsing the results for web client")


parser.add_argument('--file', '-f',
                    dest="_file_trace",
                    action="store",
                    help="Trace file which needs to be parsed",
                    required=True)

parser.add_argument('--opath', '-op',
                    dest="output_path",
                    action="store",
                    help="Directory where results should be saved",
                    required=True)

# Expt parameters
args = parser.parse_args()


cell_snr = defaultdict(lambda : defaultdict(list))
cell_rsrp = defaultdict(lambda : defaultdict(list))

def save_metric_to_file(_path,_file_name,header,*values):
 

    f_out = None
    if not os.path.isdir(_path):
        os.system('mkdir -p %s'%_path)
    
    full_path_name = os.path.join(_path, _file_name) 
    #print full_path_name
    if not os.path.isfile(full_path_name):
        f_out = open(full_path_name,'w') 
        spamwriter = csv.writer(f_out, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
        spamwriter.writerow(header)    
    else:
        f_out = open(full_path_name,'a') 
    
    spamwriter = csv.writer(f_out, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
    spamwriter.writerow(values) 
    f_out.close()   

print (args.output_path)
#infile = open("/home/darijo/workspace/ns-allinone-3.25/ns-3.25/random_static_210-no-op-DlRsrpSinrStats.txt")
infile = open(args._file_trace)
for line in infile:
    #print line
    content = re.search(REG_RSRP_SINR,line)
    if content:
       
        ue_imsi = content.group(3)
        cell_id = int(content.group(2))
        cell_snr[cell_id][content.group(1)].append(float(content.group(6)))
        rsrp_temp = np.around(10*log10(float(content.group(5))),decimals=2)
        cell_rsrp[cell_id][content.group(1)].append(rsrp_temp)
        #print ue_imsi
        save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-SINR.csv",["Time","SINR","CellID_SINR"],content.group(1),content.group(6),cell_id)

        save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-RSRP.csv",["Time","RSRP","CellID_RSRP"],content.group(1),rsrp_temp,cell_id)

        

infile.close()


pf_snr = pd.DataFrame(cell_snr)
fullname = os.path.join(args.output_path,"CELL_SINR.csv")
pf_snr.to_csv(fullname,sep=';')

pf_rsrp = pd.DataFrame(cell_rsrp)
fullname = os.path.join(args.output_path,"CELL_RSRP.csv")
pf_rsrp.to_csv(fullname,sep=';')










