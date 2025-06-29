# -*- coding: utf-8 -*-
"""This module parses simulation logs"""

import os
from argparse import ArgumentParser



parser = ArgumentParser(description="Parsing log files from ns-3 NR simulation")


parser.add_argument('--path', '-p',
                    dest="dir_path",
                    action="store",
                    help="Folder where ns-3 is installed",
                    default="/home/user1/workspace/")

parser.add_argument('--opath', '-op',
                    dest="output_path",
                    action="store",
                    help="Folder where results should be saved",
                    default='/home/user1/ns3-logs/')

parser.add_argument('--num_ue', '-nu',
                    dest="num_ue",
                    type=int,
                    action="store",
                    help="Number of UEs",
                    default=20)

parser.add_argument('--sim_time', '-st',
                    dest="sim_time",
                    type=int,
                    action="store",
                    help="Simulation time in seconds",
                    default=59)
# num_ue= 20
# simTime = 59

# Expt parameters
args = parser.parse_args()

def copy_logs(logs_location, save_path):
    trace_files = [logs_location+f for f in os.listdir(logs_location) if
                   os.path.isfile(os.path.join(logs_location, f))]
    os.system("mkdir -p %s"%save_path)
    for trace_file in trace_files:
        # if "Stats" in trace_file:
        if "Stats" in trace_file or "RxPacketTrace" in trace_file:
            os.system("cp %s %s"%(trace_file, save_path))
            # os.system("rm %s"%(trace_file))
            
def parse_mcs(path_to_logs, out_path):
    # mcs_log = os.path.join(path_to_logs, "DlTxPhyStats.txt") # lte
    mcs_log = os.path.join(path_to_logs, "NrDlMacStats.txt") # nr
    print(mcs_log)
    mcs_out_l = os.path.join(out_path, "RAW/")
    os.system("mkdir -p %s"%mcs_out_l)
    # os.system("python3 mcs2.py -f %s -op %s"%(mcs_log, mcs_out_l)) 
    os.system("python3 scratch/scripts/mcs2.py -f %s -op %s"%(mcs_log, mcs_out_l))      

def parse_CQI(path_to_logs, out_path):
    cqi_log = os.path.join(path_to_logs, "no-op-Stats.txt")
    cqi_out_l = os.path.join(out_path, "RAW/")
    os.system("mkdir -p %s"%cqi_out_l)
    # os.system("python3 UE_parsing.py -nu 60 -f %s -op %s"%(cqi_log, cqi_out_l))  #lte
    os.system("python3 scratch/scripts/UE_parsing.py -nu %d -f %s -op %s"%(args.num_ue, cqi_log, cqi_out_l))     #nr 
    # os.system("python3 scratch/scripts/UE_parsing.py -nu %d -f %s -op %s"%(num_ue, cqi_log, cqi_out_l))     #nr 

def parse_RSRP_SINR(path_to_logs, out_path):
    snr_rsrp_log = os.path.join(path_to_logs, "DlRsrpSinrStats.txt")
    sr_out_l = os.path.join(out_path, "RAW/")
    os.system("mkdir -p %s"%sr_out_l)
    os.system("python3 rsrp_sinr.py -f %s -op %s"%(snr_rsrp_log, sr_out_l))

def parse_pdcp(path_to_logs, out_path):
    # pdcp_log = os.path.join(path_to_logs, "DlPdcpStats.txt") # lte
    pdcp_log = os.path.join(path_to_logs, "NrDlPdcpStatsE2E.txt") #nr
    pdcp_out_l = os.path.join(out_path, "RAW/")
    os.system("mkdir -p %s"%pdcp_out_l)
    # os.system("python3 pdcp.py -f %s -op %s"%(pdcp_log, pdcp_out_l)) 
    os.system("python3 scratch/scripts/pdcp.py -f %s -op %s"%(pdcp_log, pdcp_out_l))                                                                    


def parse_phy(path_to_logs, out_path):
    phy_log = os.path.join(path_to_logs, "RxPacketTrace.txt") 
    phy_out_l = os.path.join(out_path, "RAW/")
    os.system("mkdir -p %s"%phy_out_l)
    os.system("python3 scratch/scripts/phy.py -nu %d -f %s -op %s"%(args.num_ue, phy_log, phy_out_l))  
    # os.system("python3 scratch/scripts/phy.py -nu %d -f %s -op %s"%(num_ue, phy_log, phy_out_l))   

def create_competing_metrics(metric, input_path, out_path):
    # os.system("python3 scratch/scripts/network_metric.py -nu %d -ms %s -p %s -op %s"%(num_ue, metric,
    os.system("python3 scratch/scripts/network_metric.py -nu %d -ms %s -p %s -op %s"%(args.num_ue, metric,
                                                                      input_path,
                                                                     out_path))
            
if __name__ == "__main__":
        
        # print(args.dir_path + "/ns-3-allinone/ns-3-dev/")
        # # copy_logs(args.dir_path + "/ns-3-allinone/ns-3-dev/",
        # #          args.output_path+"/logs/")

        print(args.dir_path + "/ns-allinone-3.42/ns-3.42/")
        copy_logs(args.dir_path + "/",
                 args.output_path+"/logs/")
        print (os.getcwd())
        print (os.path.dirname(os.path.abspath(__file__)))
        
        # parse_mcs(args.output_path + "/logs/", args.output_path)
        parse_CQI(args.output_path + "/logs", args.output_path)
        # parse_RSRP_SINR(args.output_path+"/logs/", args.output_path)
        parse_pdcp(args.output_path+"/logs/", args.output_path)
        parse_phy(args.output_path + "/logs", args.output_path)
        
        create_competing_metrics("SINR", args.output_path+"/RAW/",args.output_path+"/CMetrics/")
        create_competing_metrics("CQI", args.output_path+"/RAW/", args.output_path+"/CMetrics/")
        create_competing_metrics("THR", args.output_path+"/RAW/", args.output_path+"/CMetrics/")
        create_competing_metrics("RSRP", args.output_path+"/RAW/", args.output_path+"/CMetrics/")
                                 
                                 
        os.system("rm %s/RAW/CELL_*.csv"%args.output_path)
        # #os.system("rm %s/RAW/UE_0-*.csv"%args.output_path)
        os.system("mv %s/CMetrics/* %s/RAW/"%(args.output_path, args.output_path))
        os.system("mv %s/CMetrics %s/Positions"%(args.output_path, args.output_path))
        os.system("mv %s/RAW/UE_*-POSITION.csv %s/Positions/"%(args.output_path, args.output_path))
        
        os.system("python3 scratch/scripts/newAveraging.py -p %s/RAW/ -op %s/averaged/1s/ -il 1.0"%(args.output_path, args.output_path))
        
        os.system("python3 scratch/scripts/combine.py -p %s/averaged/1s/ -op %s/combine/1s/ -il 1.0 -nu %d -td %d -di Ach.Rate,CellID_PDCP,CellID_THR,CellID_BLER,CellID_SINR,CellID_RSRP,CellID_MCS,MCS"%(args.output_path, args.output_path, args.num_ue, args.sim_time))
        # os.system("python3 scratch/scripts/combine.py -p %s/averaged/1s/ -op %s/combine/1s/ -il 1.0 -nu %d -td %d -di Ach.Rate,CellID_PDCP,CellID_THR,CellID_BLER,CellID_SINR,CellID_RSRP,CellID_MCS,MCS"%(args.output_path, args.output_path,num_ue, simTime))
