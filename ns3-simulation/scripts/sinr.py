import os
import re
import numpy as np
import subprocess
from collections import defaultdict
from argparse import ArgumentParser
import csv
import pandas as pd
import json
from math import log10
'''Script for parsing UE performance metrics (RSRP/SINR)

    Created by Xingmei Ding, Computer Science, UCC, 23.07.2016.

    Tasks:
        - extract each metric to its own file and per user
        - group all metrics per user and align with timeline
        - export results to csv
        -
'''
# Time: 0.278214 CellId: 1 RNTI: 1 SINR (dB): 53.9767
# Time: 0.278214 CellId: 1 RNTI: 2 SINR (dB): 38.4138
# REG_SINR = r'Time:\s+([0-9]+\.[0-9]+)\s+CellId:\s+([0-9]+)\s+RNTI:\s+([0-9]+)\s+SINR\s+\(dB\):\s+([0-9]+\.[0-9]+)'

# DLDataSinr.txt
# Time	CellId	RNTI	BWPId	SINR(dB)
# 0.278214	1	1	0	52.1445
# 0.278214	1	2	0	32.7601
# REG_SINR = r'([0-9]+\.?[0-9]*)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+\.?[0-9]*)'


parser = ArgumentParser(description="Parsing the results for web client")


parser.add_argument('--numUe', '-nu',
                    dest="num_ue",
                    action="store",
                    help="Number of UE in the simulation",
                    required=True)

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

_cellid_and_rnti_to_imsi = defaultdict(lambda : defaultdict(int))

def load_mapping():
    with open('scratch/scripts/ue_mapping.json', 'r') as f:
        mapping_data = json.load(f)
    _cellid_and_rnti_to_imsi.clear()
    for cell_id, rnti_dict in mapping_data['cellid_and_rnti_to_imsi'].items():
        cell_id = int(cell_id)
        for rnti, imsi in rnti_dict.items():
            _cellid_and_rnti_to_imsi[cell_id][int(rnti)] = int(imsi)

load_mapping()

cell_sinr = defaultdict(lambda : defaultdict(list))

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

# print (args.output_path)
# infile = open(args._file_trace)

# for line in infile:
#     #print line
#     content = re.search(REG_SINR,line)
#     if content:
       
#         rnti = int(content.group(3))
#         cell_id = int(content.group(2))
#         cell_sinr[cell_id][content.group(1)].append(float(content.group(5)))
#         ue_imsi = _cellid_and_rnti_to_imsi[cell_id][rnti]
#         #sinr
#         save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-SINR.csv",["Time","SINR","CellID_SINR"],content.group(1),content.group(5),cell_id)

# infile.close()

infile = open(args._file_trace)
header_line = infile.readline() 

for line in infile:
    parts = line.strip().split('\t')
    if len(parts) != 5:
        continue 
    
    time_str, cell_id_str, rnti_str, bwpid_str, sinr_str = parts
    
    cell_id = int(cell_id_str)
    rnti = int(rnti_str)
    time = time_str
    sinr = float(sinr_str)
    
    ue_imsi = _cellid_and_rnti_to_imsi[cell_id][rnti]

    cell_sinr[cell_id][time].append(sinr)

    save_metric_to_file(args.output_path,
                        "UE_" + str(ue_imsi) + "-SINR.csv",
                        ["Time", "SINR", "CellID_SINR"],
                        time, sinr_str, cell_id)

infile.close()


pf_snr = pd.DataFrame(cell_sinr)
fullname = os.path.join(args.output_path,"CELL_SINR.csv")
pf_snr.to_csv(fullname,sep=';')











