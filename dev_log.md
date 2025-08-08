# Development Log (dev_log.md)

## 2025-08-08
**[ns3-simulation/scripts/{cqi.py,phy.py,UE_parsing.py,sinr.py,}]**
- Change the output file path of ue_mapping.json to output_path/RAW.
- Generated requirements.txt

## 2025-08-07
**[model_training/colaborative_users.py]**
- Add multi-core CPU support.

## 2025-07-31
**[ns3-simulation/5g_sim.cc]**
**[ns3-simulation/nr-mac-scheduler-cqi-management.cc]**
- Modified CQI output format from verbose descriptive text to concise tab-separated values for easier parsing.
- Removed SINR logging and related output files; now using existing DLDataSinr.txt for SINR data processing

**[ns3-simulation/scripts/sinr.py]**
**[ns3-simulation/scripts/cqi.py]**
- Updated parsing scripts for CQI and SINR metrics to use simple `split('\t')` line parsing instead of regex, leveraging the fixed tabular file format.

## 2025-07-29
**[ns3-simulation/5g_sim.cc]**
- Optimized SINR and CQI logging to reduce I/O overhead and simulation time.
- Opened log files (`NrDlSinrStats.txt`, `NrDlCqiStats.txt`) once at simulation start and closed them after simulation ends.
- Replaced `std::endl` with `"\n"` to avoid frequent flushing.

**[ns3-simulation/cqilog_stream.{h,cc}]**
- Implemented CQI logging management originally designed for `contrib/nr/model/` in ns-3.
- Added new source files to manage CQI logging globally.
- Declared `outCqiFile` as a shared output stream across modules.

**[ns3-simulation/nr-mac-scheduler-cqi-management.cc]**
- Modified CQI logging logic from the original ns-3 source file located at `contrib/nr/model/nr-mac-scheduler-cqi-management.cc`.
- Redirected CQI logging to use `outCqiFile` without opening/closing per write.
- Replaced `std::endl` with `"\n"` to avoid frequent flushing.


## 2025-07-27
**[model_training/transform_dataset_mthread_one_cell.cc]**
- Modified the multithreaded data preprocessing workflow to resolve argument passing issues in macOS multiprocessing by using an explicit config dictionary instead of relying on argparse global variables.
**[model_training/run_transf_mruns.cc]**
- run the script twice with active_only set to true and false respectively, both runs successfully.

## 2025-07-05
**[ns3-simulation/5g_sim.cc]**
- Updated antenna and beamforming configuration:
  - Set both UE and gNB antenna numbers to 1x1.
  - Switched to IsotropicAntennaModel for both UE and gNB.
  - Used IdealBeamformingHelper for beamforming configuration.

## 2025-07-01
**[model_training/colaborative_users.py]**
- Data generation/experiment:
  - Generated collaborative user data for 20 UEs over a 60-second simulation (ue20_60s).

## 2025-06-30
**[model_training/colaborative_users.py]**
- Code review/analysis:
  - Reviewed and analyzed the code logic for collaborative user statistics and feature extraction.

## 2025-06-29
**[ns3-simulation/5g_sim.cc]**
- Simulation/Experiment:
  - Ran 5g_sim.cc to generate simulation data for 20, 40, 60, 80, and 100 users, each over a 100-second duration.
  - Collected output data for subsequent analysis.

## 2025-06-28
**[ns3-simulation/5g_sim.cc]**
- Modified OnTime and OffTime duration ranges for the On-Off application:
  - Adjusted the minimum and maximum values for OnTime and OffTime attributes.
- Updated the start time range for TCP applications:
  - Changed the Min and Max values for the TCP application start time.

## 2025-06-25
**[ns3-simulation/5g_sim.cc]**
- Analysis/Observation:
  - Noted that the On-Off application mode produces data with long silence periods in the simulation output.

## 2025-06-24
**[ns3-simulation/5g_sim.cc]**
- Calculated RSRQ using numRbs, RSSI, and RSRP values:
  - Utilized g_ueIndexToRssi (std::map<uint32_t, UeRssiInfo>) to store and retrieve per-UE RSSI.
  - Performed RSRQ computation in the ReportUeMeasurementsCallback function based on the collected metrics.

## 2025-06-23
**[ns3-simulation/5g_sim.cc]**
- Added RSSI tracing and logging:
  - Implemented the UeRssiPerProcessedChunkTrace callback to capture and store per-UE RSSI values during simulation.
  - Connected the callback to the RssiPerProcessedChunk trace source for each UE.

## 2025-06-22
**[ns3-simulation/5g_sim.cc]**
- Added code to retrieve numRbs, channel bandwidth, and subcarrier spacing from the UE PHY:
  - Output these values for later use in RSRQ calculation and analysis.

## 2025-06-21
**[ns3-simulation/5g_sim.cc]**
- Continued analysis of the zero throughput issue under the On-Off application mode.
- Changed the MaxBytes attribute for dlClient to 0 to allow unlimited data transfer.

## 2025-06-20
**[ns3-simulation/5g_sim.cc]**
- Continued analysis of the zero throughput issue under the On-Off application mode.
- Modified channel settings to investigate and address the throughput problem.

## 2025-06-19
**[ns3-simulation/5g_sim.cc]**
- Analysis:
  - Investigated the issue of zero throughput observed under the On-Off application mode.
  - No code changes made; focused on data and simulation output analysis.

## 2025-06-13
**[ns3-simulation/5g_sim.cc]**
- Changed UE attachment logic:
  - Replaced AttachToClosestGnb with AttachToGnb to explicitly attach each UE to a specified gNB.

## 2025-06-12
**[ns3-simulation/scripts/network_metric.py]**
- Continued development and adaptation of the script for 5G NR:
  - Improved metric extraction and summarization logic to better handle 5G NR trace formats and data structures.

**[ns3-simulation/5g_sim.cc]**
- Added code to print RSRP and RSRQ values.
- Observed that RSRQ is always zero in the simulation output.
- Investigated contrib/nr/model/nr-ue-phy.cc and found that RSRQ is currently hardcoded to zero.

## 2025-06-11
**[ns3-simulation/scripts/network_metric.py]**
- Developed a new script for processing and summarizing network performance metrics for 5G NR:
  - Extracts and aggregates specified metrics (e.g., CQI) from per-UE trace files.
  - Computes cell-level statistics by aligning and averaging metrics across users in the same cell.
  - Outputs summarized results to CSV files for further analysis.

## 2025-06-10
**[ns3-simulation/scripts/phy.py]**
- Developed a new script for parsing UE PHY layer performance metrics from 5G NR RxPacketTrace.txt, including SINR, CQI, MCS, and TBLER extraction and CSV export.

**[ns3-simulation/scripts/UE_parsing.py]**
- Modified the script to adapt all 4G-related logic and regular expressions to support 5G NR trace formats and metrics.
- Added extraction of RSRP (Reference Signal Received Power) from 5G NR traces.

## 2025-06-09
**[ns3-simulation/scripts/mcs2.py]**
- Modified and adapted the script to support 5G NR trace formats for MCS parsing.

**[ns3-simulation/scripts/pdcp.py]**
- Modified and adapted the script to support 5G NR trace formats for PDCP throughput and delay parsing.

## 2025-06-07
**[ns3-simulation/5g_sim.cc]**
- Added TCP On-Off application module support:
  - Implemented logic to configure and install TCP On-Off applications for UEs when onOffApp is set to true.
  - Set OnTime and OffTime attributes for On-Off traffic patterns.
  - Ensured correct integration with PacketSink and bearer activation for TCP On-Off flows.
  - Added dedicated EPS bearer activation for TCP On-Off flows using NrEpcTft and NrEpsBearer to support low-latency traffic.

## 2025-06-06
**[ns3-simulation/5g_sim.cc]**
- Added support for TCP application type in the simulation:
  - Implemented logic to configure and install TCP BulkSend and PacketSink applications for UEs.
  - Added conditional checks to select between TCP and other application types.
  - Set MaxBytes attribute for BulkSendHelper to 0 (unlimited transfer).

## 2025-06-05
**[ns3-simulation/5g_sim.cc]**
- Added new callback registrations and corresponding functions:
  - Registered callbacks for UE RRC connection establishment and release events.
  - Registered mobility callback to track UE position changes.
  - Registered callback for UE measurement reporting (e.g., RSRP, RSRQ, etc.).
  - Implemented the corresponding handler functions for these events.

## 2025-06-04
**[ns3-simulation/5g_sim.cc]**
- Changed scenario configuration:
  - Switched from UMa_LoS to UMa.
- Issues found:
  - Observed that CQI values remained unchanged after the scenario switch.

## 2025-06-03
**[ns3-simulation/5g_sim.cc]**
- Changed scenario configuration:
  - Switched from UMa_LoS to UMa.
- Issues found:
  - Observed that CQI values remained unchanged after the scenario switch.

## 2025-06-02
**[ns3-simulation/5g_sim.cc]**
- Changed the beamforming configuration:
  - Replaced IdealBeamformingHelper with RealisticBeamformingHelper for more realistic beamforming modeling in the simulation.

## 2025-06-01
**[ns3-simulation/5g_sim.cc]**
- Updated simulation configuration:
  - Set the number of base stations (gNBs) to 1.
  - Modified UdpClient parameters: set MaxPackets to 100,000,000 and PacketSize to 1024.

## 2025-05-31
**[ns3-simulation/5g_sim.cc]**
- Added advanced channel configuration features to the simulation:
  - Enabled shadowing effects in the channel model.
  - Configured the scenario to UMa (Urban Macro) with Line-of-Sight (LoS) conditions.
  - Set the number of base stations (gNBs) to 1 for the simulation scenario.
  - Integrated RealisticBeamformingHelper and related channel condition updates.

## 2025-05-29
**[ns3-simulation/5g_sim.cc]**
- Added CQI (Channel Quality Indicator) recording and logging features to the simulation:
  - Implemented callback functions to trace and print CQI, RSRP, and SINR values for each UE during the simulation.
  - Enabled detailed logging for CQI management in the NR MAC scheduler.
  - Provided console output for CQI and related radio measurements for further analysis.

## 2025-05-27
**[ns3-simulation/5g_sim.cc]**
- Added throughput calculation and logging features to the simulation:
  - Implemented a function to periodically calculate and print the current throughput for each UE during the simulation.
  - Scheduled throughput reporting at 1-second intervals using Simulator::Schedule.
  - Provided detailed console output for throughput and UE configuration.

## 2025-05-26
**[ns3-simulation/5g_sim.cc]**
- Added user mobility features in the simulation:
  - Introduced support for different mobility models for UEs, including SteadyStateRandomWaypoint and GaussMarkov models, with configurable speed and area bounds.
  - Allowed static or dynamic user deployment based on maximum speed parameter.
  - Configured random position allocation for UEs within a defined area (Box), supporting both static and mobile scenarios.
  - Calculated the number of UEs based on area size and user density, and logged relevant simulation parameters.

## 2025-05-25
**[ns3-simulation/5g_sim.cc]**
- Created `5g_sim.cc` to implement a basic 5G NR simulation scenario, with the following key features:
  - Created 2 gNB (base station) nodes and 2 UE (user equipment) nodes, with specified positions and antenna heights.
  - Configured 28 GHz carrier frequency, 100 MHz bandwidth, and UMa scenario parameters.
  - Set up antenna parameters, ideal beamforming, and TDMA round-robin scheduler for both gNBs and UEs.
  - Established EPC core network, remote host, IP address assignment, and routing.
  - Configured UDP downlink applications for each UE, including packet size and sending interval.
  - Started simulation applications with a simulation time of 3 seconds.
  - Enabled NR module trace features for further analysis.
