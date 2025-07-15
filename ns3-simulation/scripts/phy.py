import os
import re
import numpy as np
import subprocess
from collections import defaultdict
from argparse import ArgumentParser
import pandas as pd
import csv
import json


'''Script for parsing UE performance metrics (RB utilisation)

    Created by Darijo Raca, MISL, Computer Science, UCC, 13.10.2016.
    Updated by Xingmei Ding, Computer Science, UCC, 09.06.2025.

    Tasks:
        - extract each metric to its own file and per user
        - group all metrics per user and align with timeline
        - export results to csv
        -
'''

# nr- RxPacketTrace.txt
REG_PHY = r'([0-9]+\.?[0-9]*)\s+(DL|UL)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+(-?[0-9]+\.?[0-9]*)\s+([0-9]+)\s+([01])\s+([01])'
# Time	direction	frame	subF	slot	1stSym	nSymbol	cellId	bwpId	rnti	tbSize	mcs	rank	rv	SINR(dB)	CQI	corrupt	TBler
# 0.113143	DL	11	3	0	1	1	1	0	21	114	0	1	0	-10.0712	0	1	1	



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
cell_snr = defaultdict(lambda : defaultdict(list))

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

infile = open(args._file_trace)
for line in infile:
    #print line
    content = re.search(REG_PHY,line)
    if content:
       
        rnti = int(content.group(10))
        cell_id = int(content.group(8))
        ue_imsi = _cellid_and_rnti_to_imsi[cell_id][rnti]
        

        cell_snr[cell_id][content.group(1)].append(float(content.group(15)))

        # cell_cqi[cell_id][content.group(1)].append(int(content.group(16)))
        achievableRate = (float(content.group(11)) * 8.0) / 0.001;
        direction = content.group(2)
        if direction == "DL":

            cell_cqi[cell_id][content.group(1)].append(int(content.group(16)))

            # mcs
            save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-MCS.csv",["Time","MCS","TBSize","RV","CellID_MCS"],str(float(content.group(1))/1000.0),content.group(12),content.group(11),content.group(14),cell_id)
        
            # sinr
            save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-SINR.csv",["Time","SINR","CellID_SINR"],content.group(1),content.group(15),cell_id)

            # cqi
            save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-CQI.csv",["Time","CQI","Ach.Rate","CellID_CQI"],content.group(1),content.group(16),achievableRate,cell_id)


            # tbler
            save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-TBLER.csv",["Time","TBLER","CellID_BLER"],content.group(1),content.group(18),cell_id)



infile.close()


pf_snr = pd.DataFrame(cell_snr)
fullname = os.path.join(args.output_path,"CELL_SINR.csv")
pf_snr.to_csv(fullname,sep=';')

pf_cqi = pd.DataFrame(cell_cqi)
fullname = os.path.join(args.output_path,"CELL_CQI.csv")
pf_cqi.to_csv(fullname,sep=';')