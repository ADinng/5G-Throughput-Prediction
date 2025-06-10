import os
import re
import numpy as np
import subprocess
from collections import defaultdict
from argparse import ArgumentParser
import csv


'''Script for parsing UE performance metrics (RB utilisation)

    Created by Darijo Raca, MISL, Computer Science, UCC, 13.10.2016.
    Updated by Xingmei Ding, Computer Science, UCC, 09.06.2025.

    Tasks:
        - extract each metric to its own file and per user
        - group all metrics per user and align with timeline
        - export results to csv
        -
'''
# lte - DlTxPhyStats.txt
# REG_MAC = r'([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+[0-9]+\s+[0-9]+\s+([0-9]+)\s+([0-9]+)\s+[0-9]+\s+([0-9]+)'
#% time	cellId	IMSI	RNTI	layer	mcs	size	rv	ndi
#313	1	1	2	0	28	4395	0	1

# Nr - NrDlMacStats.txt
REG_MAC = r'([0-9\.]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)'
# % time(s)	cellId	bwpId	IMSI	RNTI	frame	sframe	slot	symStart	numSym	harqId	ndi	rv	mcs	tbSize
# 0.111	1	0	0	21	11	3	0	1	1	15	1	0	0	114


parser = ArgumentParser(description="Parsing the results for web client")


parser.add_argument('--opath', '-op',
                    dest="output_path",
                    action="store",
                    help="Directory where results should be saved",
                    required=True)

parser.add_argument('--file', '-f',
                    dest="_file_trace",
                    action="store",
                    help="Trace file which needs to be parsed",
                    required=True)
parser.add_argument('--totalRb', '-trb',
                    dest="total_rb",
                    action="store",
                    help="Total number of RB per sector",
                    default=50.0
                    )

# Expt parameters
args = parser.parse_args()


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

print (args._file_trace)
#infile = open("/home/darijo/workspace/ns-allinone-3.25/ns-3.25/DlTxPhyStats.txt")
infile = open(args._file_trace)
for line in infile:
    #print line
    content = re.search(REG_MAC,line)
    if content:
       
        # ue_imsi = content.group(3) lte
        ue_imsi = content.group(4) # nr
        #print ue_imsi
        # save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-MCS.csv",["Time","MCS","TBSize","NDI","CellID_MCS"],str(float(content.group(1))/1000.0),content.group(4),content.group(5),content.group(6),content.group(2))
        save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-MCS.csv",["Time","MCS","TBSize","NDI","CellID_MCS"],str(float(content.group(1))/1000.0),content.group(14),content.group(15),content.group(12),content.group(2))

        

infile.close()













