# 5G Throughput Prediction Project

This project simulates 5G multi-user scenarios using ns-3, collects network performance data, and processes the data into structured CSV files for machine learning analysis.

## Project Overview
- **Simulation:** 5G network scenarios are simulated using ns-3 (main file: `5g_sim.cc`).
- **Data Aggregation:** Raw simulation logs are processed and combined into CSV files.
- **Collaborative Feature Generation:** Collaborative statistics are computed for each user.
- **Time-Series Dataset Preparation:** Final datasets are generated for ML model training.

## Directory Structure
```
ns3-simulation/
  ├── 5g_sim.cc                  # Main 5G ns-3 simulation file
  ├── 5g_sim_channel.cc          # Channel simulation logic
  ├── 5g_sim_cqi.cc              # CQI-related simulation
  ├── 5g_max.cc, 5g_min.cc, 5g_mid.cc  # Other scenario scripts
  ├── nr-gnb-mac.cc, nr-mac-scheduler-cqi-management.cc  # ns-3 MAC layer logic
  └── scripts/                   # Python scripts for parsing and processing 
      ├── rsrp_sinr.py
      ├── mcs2.py
      ├── parse_logs.py
      ├── UE_parsing.py
      ├── phy.py
      ├── newAveraging.py
      ├── combine.py
      ├── pdcp.py
      ├── network_metric.py
      ├── ue_mapping.json        # Mapping/configuration file
      └── ... (other helper scripts)

model_training/
  ├── colaborative_users.py                  # Collaborative feature statistics
  ├── transform_dataset_mthread_one_cell.py  # Time-series sample generation
  ├── run_ml.py                             # ML model training and evaluation
  ├── run_transf_mruns.py                   # Batch time-series transformation
  ├── run_runml_mruns.py                    # Batch ML experiments
  └── ... (other training/processing scripts)

5g_script_output/
  └── ... (CSV files generated by scripts, e.g. per-user logs, combined logs)

dataset_network/
  └── ... (collaborative feature CSVs generated from scripts)

ready_for_training/
  └── ... (final time-series datasets for ML training)
```

## Data Flow
1. **Simulation:**
   - Run `5g_sim.cc` in `ns3-simulation/` to generate raw per-user log files.
2. **Log Parsing and Aggregation:**
   - Run `ns3-simulation/scripts/parse_logs.py` to process and combine the raw logs into CSV files.
   - The intermediate and combined CSV files are stored in `5g_script_output/`.
3. **Collaborative Feature Generation:**
   - Run `model_training/colaborative_users.py` to compute collaborative statistics and output to `dataset_network/`.
4. **Time-Series Dataset Preparation:**
   - Run `model_training/transform_dataset_mthread_one_cell.py` to generate final ML-ready datasets in `ready_for_training/`.

## Main Scripts
- **5g_sim.cc**: Main ns-3 simulation file for 5G scenarios.
- **colaborative_users.py**: Aggregates per-user logs, computes collaborative statistics (min/max/mean/std among randomly selected users), and outputs collaborative feature CSVs.
- **transform_dataset_mthread_one_cell.py**: Converts collaborative feature CSVs into time-series samples for ML model training, supporting sliding window, history/horizon, and active/inactive sample selection.

## Basic Usage
1. **Run ns-3 Simulation**
   - Compile and run `5g_sim.cc` in the `ns3-simulation` directory to generate raw per-user log files.
2. **Process Data**
   - Use the scripts in `scripts/model_training/` to process the raw logs:
     - Generate collaborative features:
       ```bash
       python colaborative_users.py --folder_path <input_log_folder> --output_path ../dataset_network/ --percent_comp_users 50 --num_runs 10
       ```
     - Generate time-series samples:
       ```bash
       python transform_dataset_mthread_one_cell.py --folder_path ../dataset_network/ --output_path ../ready_for_training/ --target_metric THR --history 10 --horizon 1 --active_only True
       ```
3. **Train ML Models**
   - Use the processed datasets in `ready_for_training/` for your machine learning experiments.

## Notes
- The 5G simulation does **not** collect `PDCP_Throughput` and `Delay` metrics, so these features are not included in the collaborative statistics.
- All scripts are designed to be modular and can be adapted for different simulation setups or feature requirements.

## Contact
For questions or contributions, please open an issue or contact the maintainer.
