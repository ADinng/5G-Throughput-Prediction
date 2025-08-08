import os
import pandas as pd

def analyze_folder(folder):
    total_rows = 0
    for fname in os.listdir(folder):
        if fname.endswith('-comb.csv'):
            df = pd.read_csv(os.path.join(folder, fname))
            cqi = df['CQI'].dropna()
            if len(cqi) == 0:
                continue
            cqi_min = cqi.min()
            cqi_max = cqi.max()
            cqi_mean = cqi.mean()
            total_rows += len(df)
            print(f"{fname}: CQI min={cqi_min}, max={cqi_max}, mean={cqi_mean:.2f}")




print("====0. ns3_ue20_100s_onoff====")
analyze_folder('5g_script_output/ns3_ue20_100s_onoff/combine/1s')

# print("====1. ns3_ue40_1000s_onoff====")
# analyze_folder('5g_script_output/ns3_ue40_1000s_onoff/combine/1s')

print("====2. ns3_ue20_113s_onoff_tdmaPF====")
analyze_folder('5g_script_output/ns3_ue20_113s_onoff_tdmaPF/combine/1s')

# print("====2.ns3_ue20_100s_onoff_noconfig400====")
# analyze_folder('5g_script/ns3_ue20_100s_onoff_noconfig400/combine/1s')

# print("====2.ns3_ue20_100s_onoff_noconfig380====")
# analyze_folder('5g_script/ns3_ue20_100s_onoff_noconfig380/combine/1s')