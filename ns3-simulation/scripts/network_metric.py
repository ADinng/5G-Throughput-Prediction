from argparse import ArgumentParser
import pandas as pd
import os
import numpy as np
import math
import csv
import ast
import multiprocessing as MP
parser = ArgumentParser(description="Taking all the performance metrics from competing UEs at the same cell")


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
                    default=100)

parser.add_argument('--metrics', '-ms',
                    dest="_metrics",
                    action="store",
                    help="List of metrics that need to be processed (separated by comma (',')",
                    default='CQI')

# Expt parameters
args = parser.parse_args()

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

def summarize_parameters(df,metric,ue_prefix):

    _name = "CELL_" + metric + '.csv'
    _fullname = os.path.join(args.dir_path,_name)
    new_pdf = pd.read_csv(_fullname,sep=';',index_col=0)
    #print new_pdf.get_value(0.266 + 0.01,'1')
    #print new_pdf
    comp_cl = df.iloc[:,[0,1,-1]]
    for row in comp_cl.itertuples():
        curr_time = row[1]
        cellid = str(row[-1])
        val = new_pdf.loc[curr_time,cellid]
        dd = ''.join(val[1:-1].split())
        fin = list(map(float, dd.split(',')))
        # fin = list(map(float, ast.literal_eval(val)))
        fin = np.around(fin,decimals=3).tolist()
        cqi_curr = np.around(float(row[2]),decimals=3)
        #print cqi_curr
        try:
            fin.remove(cqi_curr)
        except:
            pass
        #print fin
        #print len(fin)
        #print curr_time
        ff_a = []
        if metric == metric:
            step_back_time = float(curr_time) - 0.001
            step_forward_time = float(curr_time) + 0.001
        
        
            if step_forward_time in new_pdf.index:
                val = new_pdf.loc[step_forward_time,cellid]
                if isinstance(val, str):
                    f_a = ''.join(val[1:-1].split())
                    ff_a = list(map(float, f_a.split(',')))
                    # ff_a = list(map(float, ast.literal_eval(val)))
                 #print ff_a
            elif step_back_time in new_pdf.index:
                val = new_pdf.loc[step_back_time,cellid]
                if isinstance(val, str):
                    f_a = ''.join(val[1:-1].split())
                    ff_a = list(map(float, f_a.split(',')))
                    # ff_a = list(map(float, ast.literal_eval(val)))

        fin = fin + ff_a
        if len(fin) > 0:
            final_value = np.mean(fin)
        else:
            final_value = 0
        save_metric_to_file(args.output_path,ue_prefix+"C"+metric+".csv",["Time","CellID_"+metric,"C"+metric],curr_time,cellid,final_value)


def process_ue(i):
    ue_prefix = "UE_" + str(i+1) +"-"
    metrics = args._metrics.split(',')
    
    for metric in metrics:
        name = ue_prefix + metric + '.csv'
        fullname = os.path.join(args.dir_path,name)
          
        if os.path.exists(fullname):
            pdf = pd.read_csv(fullname)
            summarize_parameters(pdf,metric,ue_prefix)


# for i in range(int(args.num_ue)):
#     ue_prefix = "UE_" + str(i+1) +"-"
#     f_names = []
#     # parse metrics of interest
#     metrics = args._metrics.split(',')
    
#     for metric in metrics:
#         name = ue_prefix + metric + '.csv'
#         fullname = os.path.join(args.dir_path,name)
          
#         if os.path.exists(fullname):
#             pdf = pd.read_csv(fullname)
#             summarize_parameters(pdf,metric,ue_prefix)
            
if __name__ == '__main__':
    num_ues = int(args.num_ue)
    with MP.Pool(MP.cpu_count()-3) as pool:
        pool.map(process_ue, range(num_ues))
    
