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

REG_PDCP = r'([0-9]+\.?[0-9]*)\s+([0-9]+\.?[0-9]*)\s+([0-9]+)\s+([0-9]+)\s+[0-9]+\s+[0-9]+\s+[0-9]+\s+([0-9]+)\s+[0-9]+\s+([0-9]+)\s+([0-9]+\.?[0-9]+)'

# lte- DLPdcpstats.txt
#% start	end	CellId	IMSI	RNTI	LCID	nTxPDUs	TxBytes	nRxPDUs	RxBytes	delay	stdDev	min	max	PduSize	stdDev	min	max
#   0.25	0.5	1	1	7	4	95	100130	21	22134	0.0650943	0.0212789	0.011999	0.094999	1054	0	1054	1054	

# Nr - NrDlPdcpStatsE2E.txt
# % start(s)	end(s)	CellId	IMSI	RNTI	LCID	nTxPDUs	TxBytes	nRxPDUs	RxBytes	delay(s)	stdDev(s)	min(s)	max(s)	PduSize	stdDev	min	max
# 0	0.25	1	1	1	4	5	290	4	232	0.030685	0.048135	0.00284576	0.102774	58	0	58	58	


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

print (args.output_path)
#infile = open("/home/darijo/workspace/ns-allinone-3.25/ns-3.25/DlPdcpStats.txt")

infile = open(args._file_trace)
for line in infile:
    #print line
    content = re.search(REG_PDCP,line)
    if content:
       
        ue_imsi = content.group(4)
        #print ue_imsi
        epoc_interval = float(content.group(2))- float(content.group(1)) 
        thr = (int(content.group(6))*8.0)/epoc_interval
        save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-PDCP.csv",["Time","PDCP_Throughput","Delay","CellID_PDCP"],content.group(2),np.around(thr,decimals=2),content.group(7),content.group(3))

        

infile.close()













