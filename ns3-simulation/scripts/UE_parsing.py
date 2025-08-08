import os
import re
import numpy as np
import subprocess
from collections import defaultdict
from argparse import ArgumentParser
import csv
import pandas as pd
import json
from pathlib import Path

'''Script for parsing UE performance metrics (throughput, CQI, SNR...)

    Created by Darijo Raca, MISL, Computer Science, UCC, 04.10.2016.
    Updated by Xingmei Ding, Computer Science, UCC, 09.06.2025.

    Tasks:
        - extract each metric to its own file and per user
        - group all metrics per user and align with timeline
        - export results to csv
        -
'''

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



# we need to store mapping between UE ID and CellID and RNTI
# this is needed in order to match throughput values and rest of the metrics
# IMSI is the glue between the pair
# with mobility, we need to update relation between imsi and cellid and rnti

_id_to_imsi = defaultdict(int)
_cellid_and_rnti_to_imsi = defaultdict(lambda : defaultdict(int))


#
cell_cqi = defaultdict(lambda : defaultdict(list))
cell_thr = defaultdict(lambda : defaultdict(list))
cell_rsrp = defaultdict(lambda : defaultdict(list))
# cell_rsrq = defaultdict(lambda : defaultdict(list))
cell_rssi = defaultdict(lambda : defaultdict(list))


# load on each cell in terms of number of users connected to it
_cellid_load = defaultdict(int)
_cellid_load_backup = defaultdict(int)
_imsi_connected = defaultdict(int)

# dict of UE IMSI and times when connected to a base station
_time_start = defaultdict(float)
# dict of UE IMSI and duration of time connected to a base station
_time_spent = defaultdict(float)

_leaving_cell = defaultdict(float)

REG_ID_IMSI_CELLID_MAP = r'([0-9]+\.?[0-9]*) /NodeList/([0-9]+)/DeviceList/[0-9]+/NrUeRrc/ConnectionEstablished UE\s+IMSI\s+([0-9]+).*CellId\s+([0-9]+).*RNTI\s+([0-9]+)'
REG_CQI = r'.*Time:\s+([0-9]+\.?[0-9]*)\s+RNTI\s+=\s+([0-9]+)\s+CellID\s+=\s+([0-9]+)\s+achievableRate = ([0-9]+\.?[0-9]*\w?\+?[0-9]*).*CQI\s+([0-9]+)'
REG_TBLER = r'.*Time:\s+([0-9]+\.?[0-9]*)\s+RNTI\s+([0-9]+)\s+CellID:\s+([0-9]+).*TBLER\s+([0-9]+\.?[0-9]*)'
REG_THR = r'.*Time:\s+([0-9]+\.?[0-9]*)\s+UE\s+([0-9]+).*Current Throughput:\s+([0-9]+\.?[0-9]*\w?\+?[0-9]*) bit/s.*'
REG_START_HAND = r'([0-9]+\.?[0-9]*)\s+/NodeList/.*HandoverStart eNB CellId ([0-9]+): start handover of UE with IMSI ([0-9]+) RNTI ([0-9]+) to CellId ([0-9]+)'
REG_END_HAND = r'([0-9]+\.?[0-9]*)\s+/NodeList/.*eNB CellId ([0-9]+): completed handover of UE with IMSI ([0-9]+) RNTI ([0-9]+)'
# 7.42521 /NodeList/11/DeviceList/0/$ns3::LteNetDevice/$ns3::LteEnbNetDevice/LteEnbRrc/NotifyConnectionRelease UE IMSI 24: released from CellId 12 with RNTI 9 #lte
# 0.033 /NodeList/0/DeviceList/0/$ns3::NrGnbNetDevice/NrGnbRrc/NotifyConnectionRelease UE IMSI 0: released from CellId 1 with RNTI 13 #nr
REG_CONN_REL = r'([0-9]+\.?[0-9]*)\s+/NodeList/.*NotifyConnectionRelease UE IMSI ([0-9]+): released from CellId ([0-9]+) with RNTI ([0-9]+)'

REG_POS_UE = r'Time: ([0-9]+\.?[0-9]*) /NodeList/([0-9]+)/.*/CourseChange POS: x=(-?[0-9]+\.?[0-9]*), y=(-?[0-9]+\.?[0-9]*), z=(-?[0-9]+\.?[0-9]*)'

# 2.6  RNTI 1, CellId: 1, Serving Cell: 1, RSRP: -144.698, RSRQ: 0
# REG_RSRQ_RSRP = r'([0-9]+\.?[0-9]*)\s+RNTI\s+([0-9]+),\s+CellId:\s+([0-9]+),\s+Serving Cell:\s+([0-9]+),\s+RSRP:\s+(-?[0-9]+\.?[0-9]*),\s+RSRQ:\s+(-?[0-9]+\.?[0-9]*)'
REG_RSRQ_RSSI = r'([0-9]+\.?[0-9]*)\s+RNTI\s+([0-9]+),\s+CellId:\s+([0-9]+),\s+Serving Cell:\s+([0-9]+),\s+RSRP:\s+(-?[0-9]+\.?[0-9]*),\s+RSSI:\s+(-?[0-9]+\.?[0-9]*)'
REG_CELL_BUFFERS = r'([0-9]+\.?[0-9]+)\s+RNTI:\s+([0-9]+)\s+Buffer Tx Level:\s+([0-9]+)\s+Buffer TxHol Level:\s+([0-9]+)\s+Buffer Rx Level: ([0-9]+)\s+Buffer RxHol Level: ([0-9]+)\s+PDU: ([0-9]+)'  
#s = '0.644214 /NodeList/5/DeviceList/0/LteEnbRrc/HandoverEndOk eNB CellId 6: completed handover of UE with IMSI 65 RNTI 8'
#s= 'Time: 0.7 UE 225 Current Throughput: 2.4576e+06 bit/s'
#0.640001 /NodeList/3/DeviceList/0/LteEnbRrc/HandoverStart eNB CellId 4: start handover of UE with IMSI 65 RNTI 12 to CellId 6
def save_metric_to_file(_path,_file_name,header,*values):


    f_out = None
    if not os.path.isdir(_path):
        os.system('mkdir -p %s'%_path)

    full_path_name = os.path.join(_path, _file_name)
    if not os.path.isfile(full_path_name):
        f_out = open(full_path_name,'w')
        spamwriter = csv.writer(f_out, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
        spamwriter.writerow(header)
    else:
        f_out = open(full_path_name,'a')

    spamwriter = csv.writer(f_out, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
    spamwriter.writerow(values)
    f_out.close()




def maping_id_imsi(_id,imsi):
    '''
        function maps UE ID and IMSI
    '''
    _id_to_imsi[_id] = imsi

def maping_cell_and_rnti_imsi(rnti,cellid,imsi):
    '''
        function maps UE ID and IMSI
    '''
    
    _cellid_and_rnti_to_imsi[cellid][rnti] = imsi
    # print(f"Establish mappings: cell_id={cellid}, rnti={rnti}, imsi={imsi}")
#open("./random/no-op.txt")
ue_counter = 0
#infile = open("/home/darijo/workspace/ns-allinone-3.25/ns-3.25/random_static_210-no-op.txt")
infile = open(args._file_trace)
for line in infile:
        content = re.search(REG_ID_IMSI_CELLID_MAP,line)
        if content:
            #print line
            ue_counter+=1
            if True:
                maping_id_imsi(int(content.group(2)),int(content.group(3)))
                maping_cell_and_rnti_imsi(int(content.group(5)),int(content.group(4)),int(content.group(3)))
                _time_start[int(content.group(3))] = float(content.group(1))
                _leaving_cell[int(content.group(3))] = int(content.group(4))
                #print "IMSI: " + str(_imsi_connected[int(content.group(3))])
                if _imsi_connected[int(content.group(3))] == 0:

                    _cellid_load[int(content.group(4))] = _cellid_load[int(content.group(4))] + 1

                    #print content.group(1)
                    save_metric_to_file(args.output_path,"CELL_LOAD.csv",["Time","CellID","Users"],content.group(1),content.group(4),_cellid_load[int(content.group(4))])
                    _imsi_connected[int(content.group(3))] = int(content.group(4))
        elif re.search(REG_CQI,line):
            new_data = re.search(REG_CQI,line)

            rnti = int(new_data.group(2))
            cell_id = int(new_data.group(3))
            cell_cqi[cell_id][new_data.group(1)].append(int(new_data.group(5)))
            #print cell_cqi
            ue_imsi = _cellid_and_rnti_to_imsi[cell_id][rnti]
            #print new_data.group(1)
            #comp_ues= ';'.join(str(x) for x in _cellid_and_rnti_to_imsi[cell_id].values() if x!=ue_imsi)
            
            # save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-CQI.csv",["Time","CQI","Ach.Rate","CellID_CQI"],new_data.group(1),new_data.group(5),new_data.group(4),cell_id)

        elif re.search(REG_TBLER,line):
            new_data = re.search(REG_TBLER,line)

            rnti = int(new_data.group(2))
            cell_id = int(new_data.group(3))

            ue_imsi = _cellid_and_rnti_to_imsi[cell_id][rnti]
            #comp_ues= ';'.join(str(x) for x in _cellid_and_rnti_to_imsi[cell_id].values() if x!=ue_imsi)

            #save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-TBLER.csv",["Time","TBLER","CellID_BLER"],new_data.group(1),new_data.group(4),cell_id)
        
        elif re.search(REG_CELL_BUFFERS,line):
            new_data = re.search(REG_CELL_BUFFERS,line)

            rnti = int(new_data.group(2))
            cell_id = 1

            ue_imsi = _cellid_and_rnti_to_imsi[cell_id][rnti]
            #comp_ues= ';'.join(str(x) for x in _cellid_and_rnti_to_imsi[cell_id].values() if x!=ue_imsi)
            save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-BUFFERS.csv",["Time","Tx", "CellID_BF"],new_data.group(1),new_data.group(3),cell_id)

        elif re.search(REG_THR,line):
            new_data = re.search(REG_THR,line)

            ue = int(new_data.group(2))

            
            ue_imsi =_id_to_imsi[ue]
            _cellid = _imsi_connected[ue_imsi]

            cell_thr[_cellid][new_data.group(1)].append(float(new_data.group(3)))
            load = _cellid_load[_cellid] - 1 
            #comp_ues= ';'.join(str(x) for x in _cellid_and_rnti_to_imsi[cell_id].values() if x!=ue_imsi)
            save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-THR.csv",["Time","THR","Load","CellID_THR"],new_data.group(1),new_data.group(3),load,_cellid)
        elif re.search(REG_START_HAND,line):
            new_data = re.search(REG_START_HAND,line)
            '''
            old_cellid = int(new_data.group(2))
            new_cellid = int(new_data.group(5))
            _cellid_load[new_cellid] = _cellid_load[new_cellid] + 1
            _cellid_load[old_cellid] = _cellid_load[old_cellid] - 1
            save_metric_to_file(args.output_path,"CELL_LOAD.csv",["Time","CellID","Users"],new_data.group(1),new_cellid,_cellid_load[new_cellid])
            save_metric_to_file(args.output_path,"CELL_LOAD.csv",["Time","CellID","Users"],new_data.group(1),old_cellid,_cellid_load[old_cellid])
            '''
        
        elif re.search(REG_CONN_REL,line):
            new_data = re.search(REG_CONN_REL,line)
            _time = new_data.group(1)
            _imsi = int(new_data.group(2))
            old_cellid = int(new_data.group(3))
            rnti = int(new_data.group(4))
            # print(_time, _imsi, old_cellid, rnti)
            if _imsi > 0:
                _cellid_load[old_cellid] = _cellid_load[old_cellid] - 1
                save_metric_to_file(args.output_path,"CELL_LOAD.csv",["Time","CellID","Users"],_time,old_cellid,_cellid_load[old_cellid])
        
        
        elif re.search(REG_POS_UE,line):
            new_data = re.search(REG_POS_UE,line)

            ue_id = int(new_data.group(2))
            x_pos = new_data.group(3)
            y_pos = new_data.group(4)
            if ue_id in _id_to_imsi:
                ue_imsi = _id_to_imsi[ue_id]
            
                save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-POSITION.csv",["Time","X_axis","Y_axis"],new_data.group(1),x_pos,y_pos)
            else:
                # print ("No association")
                print(f"No association for UE {ue_id}")
        elif re.search(REG_RSRQ_RSSI,line):
            new_data = re.search(REG_RSRQ_RSSI,line)

            # rnti = int(new_data.group(3)) #lte
            # cell_id = int(new_data.group(4)) #lte
            rnti = int(new_data.group(2))
            cell_id = int(new_data.group(3))
            
            # cell_rsrp[cell_id][new_data.group(1)].append(np.around(10*log10(float(new_data.group(5)),decimals=2))) #lte
            # # RSRP is already in dBm (log scale), no need to apply 10*log10 conversion.

            rsrp_temp = float(np.around(float(new_data.group(5)),decimals=2))
            cell_rsrp[cell_id][new_data.group(1)].append(rsrp_temp) 
        
            # cell_rsrq[cell_id][new_data.group(1)].append(new_data.group(6))
            rssi_temp = float(np.around(float(new_data.group(6)),decimals=2))
            cell_rssi[cell_id][new_data.group(1)].append(rssi_temp)

            ue_imsi = _cellid_and_rnti_to_imsi[cell_id][rnti]
            #comp_ues= ';'.join(str(x) for x in _cellid_and_rnti_to_imsi[cell_id].values() if x!=ue_imsi)
            
            # save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-RSRQ_RSRP.csv",["Time","RSRP","RSRQ","CellID_RSRQ"],new_data.group(1),rsrp_temp,new_data.group(6),cell_id)

            save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-RSRP.csv",["Time","RSRP","CellID_RSRP"],new_data.group(1),rsrp_temp,cell_id)

            save_metric_to_file(args.output_path,"UE_"+str(ue_imsi)+"-RSSI.csv",["Time","RSSI","CellID_RSSI"],new_data.group(1),rssi_temp,cell_id)

        elif re.search(REG_END_HAND,line):
            
            new_data = re.search(REG_END_HAND,line)

            _cellid_load[int(new_data.group(2))] = _cellid_load[int(new_data.group(2))] + 1
            save_metric_to_file(args.output_path,"CELL_LOAD.csv",["Time","CellID","Users"],new_data.group(1),int(new_data.group(2)),_cellid_load[int(new_data.group(2))])
            
            #_time_spent[int(new_data.group(3))] = float(new_data.group(1)) - _time_start[int(new_data.group(3))]
            #save_metric_to_file(args.output_path,"UE_"+str(int(new_data.group(3)))+"-TIMESPENT.csv",["Time","Cell","Duration"],new_data.group(1),_leaving_cell[int(new_data.group(3))],_time_spent[int(new_data.group(3))])
            _leaving_cell[int(new_data.group(3))] = int(new_data.group(2))
            _time_spent[int(new_data.group(3))] = 0
            _time_start[int(new_data.group(3))] = float(new_data.group(1))
            old_cell = _imsi_connected[int(new_data.group(3))]
            for rnti_key in _cellid_and_rnti_to_imsi[old_cell]:
                if  _cellid_and_rnti_to_imsi[old_cell][rnti_key] == int(new_data.group(3)):
                    del _cellid_and_rnti_to_imsi[old_cell][rnti_key]
                    break
            maping_cell_and_rnti_imsi(int(new_data.group(4)),int(new_data.group(2)),int(new_data.group(3)))
            _imsi_connected[int(new_data.group(3))] = int(new_data.group(2))
            #print line

# pf_cqi = pd.DataFrame(cell_cqi)
# fullname = os.path.join(args.output_path,"CELL_CQI.csv")
# pf_cqi.to_csv(fullname,sep=';')

pf_rsrp = pd.DataFrame(cell_rsrp)
fullname = os.path.join(args.output_path,"CELL_RSRP.csv")
pf_rsrp.to_csv(fullname,sep=';')

#pf_rsrq = pd.DataFrame(cell_rsrq)
#fullname = os.path.join(args.output_path,"CELL_RSRQ.csv")
#pf_rsrq.to_csv(fullname,sep=';')

pf_rssi = pd.DataFrame(cell_rssi)
fullname = os.path.join(args.output_path,"CELL_RSSI.csv")
pf_rssi.to_csv(fullname,sep=';')

pf_thr = pd.DataFrame(cell_thr)
fullname = os.path.join(args.output_path,"CELL_THR.csv")
pf_thr.to_csv(fullname,sep=';')


def save_mapping(mapping_file_path):
    mapping_data = {
        'cellid_and_rnti_to_imsi': {
            str(cell_id): {str(rnti): imsi for rnti, imsi in rnti_dict.items()}
            for cell_id, rnti_dict in _cellid_and_rnti_to_imsi.items()
        }
    }
    # with open('scratch/scripts/ue_mapping.json', 'w') as f:
    mapping_file_path.parent.mkdir(parents=True, exist_ok=True)  
    with open(mapping_file_path, 'w') as f:
        json.dump(mapping_data, f)

mapping_file_path = Path(args.output_path) / 'ue_mapping.json'
save_mapping(mapping_file_path) 

#print _cellid_and_rnti_to_imsi[12]
#print ue_counter
