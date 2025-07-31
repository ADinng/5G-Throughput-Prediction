import os
import re
import numpy as np
import subprocess
from collections import defaultdict
from argparse import ArgumentParser
import csv
import pandas as pd
import json


'''Script for parsing UE performance metrics (RB utilisation)

    Updated by Xingmei Ding, Computer Science, UCC, 23.07.2025.

    Tasks:
        - extract each metric to its own file and per user
        - group all metrics per user and align with timeline
        - export results to csv
        -
'''

# Nr - NrDlCqiStats.txt
# REG_CQI = r'.*Time:\s+([0-9]+\.?[0-9]*)\s+RNTI\s+=\s+([0-9]+)\s+CellID\s+=\s+([0-9]+)\s+achievableRate = ([0-9]+\.?[0-9]*\w?\+?[0-9]*).*CQI\s+([0-9]+)'
# Time: 0.285 RNTI = 1 CellID = 1 achievableRate = 32000 wideband CQI 15 reported


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
parser.add_argument('--totalRb', '-trb',
                    dest="total_rb",
                    action="store",
                    help="Total number of RB per sector",
                    default=50.0
                    )

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

cell_cqi = defaultdict(lambda : defaultdict(list))

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
#     content = re.search(REG_CQI,line)
#     if content:
       
#         rnti = int(content.group(2))
#         cell_id = int(content.group(3))
#         cell_cqi[cell_id][content.group(1)].append(int(content.group(5)))
#         ue_imsi = _cellid_and_rnti_to_imsi[cell_id][rnti]
#         save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-CQI.csv",["Time","CQI","Ach.Rate","CellID_CQI"],content.group(1),content.group(5),content.group(4),cell_id)

# infile.close()

infile = open(args._file_trace)
header_line = infile.readline() 

# NrDlCqiStats.txt
# Time	RNTI	CellID	AchievableRate	WidebandCQI
# 0.285	1	1	32000	15
for line in infile:
    parts = line.strip().split('\t')
    if len(parts) != 5:
        continue 
    
    time_str, rnti_str, cell_id_str, AchievableRate_str, cqi_str = parts
    
    cell_id = int(cell_id_str)
    rnti = int(rnti_str)
    time = time_str
    cqi = float(cqi_str)
    AchievableRate = int(AchievableRate_str)
    
    ue_imsi = _cellid_and_rnti_to_imsi[cell_id][rnti]

    cell_cqi[cell_id][time].append(cqi)

    save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-CQI.csv",["Time","CQI","Ach.Rate","CellID_CQI"],time,cqi,AchievableRate,cell_id)


infile.close()


pf_cqi = pd.DataFrame(cell_cqi)
fullname = os.path.join(args.output_path,"CELL_CQI.csv")
pf_cqi.to_csv(fullname,sep=';')











