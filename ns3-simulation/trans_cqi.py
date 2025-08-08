import re

input_path = "5g_script/ns3_ue60_1000s_onoff/logs/NrDlCqiStats.txt"
output_path = "5g_script/ns3_ue60_1000s_onoff/logs/NrDlCqiStats_parsed.txt"

pattern = re.compile(
    r"Time: ([\d.]+) RNTI = (\d+) CellID = (\d+) achievableRate = (\d+) wideband CQI (\d+) reported"
)

with open(input_path, "r") as fin, open(output_path, "w") as fout:
    fout.write("Time\tRNTI\tCellID\tAchievableRate\tWidebandCQI\n")  # 写表头
    for line in fin:
        match = pattern.search(line)
        if match:
            time, rnti, cellid, rate, cqi = match.groups()
            fout.write(f"{time}\t{rnti}\t{cellid}\t{rate}\t{cqi}\n")

print(f"Done. Output written to: {output_path}")
