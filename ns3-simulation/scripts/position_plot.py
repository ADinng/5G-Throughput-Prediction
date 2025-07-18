import os
import re
import pandas as pd
import matplotlib.pyplot as plt
from argparse import ArgumentParser


parser = ArgumentParser(description="Batch plot UE positions")

parser.add_argument('--path', '-p',
                    dest="dir_path",
                    action="store",
                    help="Input Positions directory",
                    required=True)

parser.add_argument('--opath', '-op',
                    dest="output_path",
                    action="store",
                    help="Output directory for trajectory plots",
                    required=True)

args = parser.parse_args()



def plot_ue_trajectory(df, out_png, ue_title='UE Movement Trajectory'):
    plt.figure(figsize=(8, 6))
    plt.plot(df['X_axis'], df['Y_axis'], marker='o', linestyle='-', color='b', label='UE Trajectory')
    plt.scatter(df['X_axis'].iloc[0], df['Y_axis'].iloc[0], color='g', label='Start', zorder=5)
    plt.scatter(df['X_axis'].iloc[-1], df['Y_axis'].iloc[-1], color='r', label='End', zorder=5)
    plt.scatter(0, 0, color='black', marker='^', s=100, label='gNB (0,0)', zorder=6)
    plt.xlabel('X Axis (m)')
    plt.ylabel('Y Axis (m)')
    plt.title(ue_title)
    plt.legend()
    plt.grid(True)
    plt.axis('equal')
    plt.tight_layout()
    plt.savefig(out_png, dpi=300)
    plt.close()


input_dir = args.dir_path
output_dir = args.output_path

if not os.path.isdir(output_dir):
    os.system('mkdir -p %s' % output_dir)
# df = pd.read_csv('5g_script/ns3_ue2_100s_onoff/Positions/UE_1-POSITION.csv')
# plot_ue_trajectory(df, 'UE_1-Trajectory.png', ue_title='UE 1 Movement Trajectory')

for filename in os.listdir(input_dir):
    if re.match(r'UE_\d+-POSITION\.csv$', filename):
        ue_id = re.findall(r'UE_(\d+)-POSITION\.csv', filename)[0]
        input_path = os.path.join(input_dir, filename)
        output_path = os.path.join(output_dir, f'UE_{ue_id}-Trajectory.png')

        df = pd.read_csv(input_path)
        plot_ue_trajectory(df, output_path, ue_title=f'UE {ue_id} Movement Trajectory')
        # print(f'Plotted trajectory for UE {ue_id}: {output_path}')