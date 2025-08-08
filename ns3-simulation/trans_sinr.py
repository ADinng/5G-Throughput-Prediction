import re

input_path = "5g_script/ns3_ue60_1000s_onoff/logs/NrDlSinrStats.txt"
output_path = "5g_script/ns3_ue60_1000s_onoff/logs/NrDlSinrStats_parsed.txt"

pattern = re.compile(
    r"Time: ([\d.]+) CellId: (\d+) RNTI: (\d+) SINR \(dB\): ([\d.eE+-]+)"
)

with open(input_path, "r") as fin, open(output_path, "w") as fout:
    fout.write("Time\tCellId\tRNTI\tBWPId\tSINR(dB)\n")  
    for line in fin:
        match = pattern.search(line)
        if match:
            time, cellid, rnti, sinr = match.groups()
            fout.write(f"{time}\t{cellid}\t{rnti}\t0\t{sinr}\n")  # BWPId 填 0



print(f"Done. Output written to: {output_path}")
