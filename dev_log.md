# Development Log (dev_log.md)

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
