# issue_log.md

## 📅 2025-07-29

### Simulation runs too slow with 60 UEs and 1000 seconds duration

#### Problem
When running a simulation with 60 UEs over 1000 seconds, the runtime was significantly longer than expected.

#### Root Cause
- The output files `NrDlCqiStats.txt` and `NrDlSinrStats.txt` grew larger than **10 GB**.
- Logging code was inefficient:
  - Opened and closed the output file **every time a line was written**.
  - Used `std::endl`, which forces the output stream to flush on every line.

This resulted in **heavy disk I/O**, severely affecting performance.

#### Solution

##### General Fixes
- Files are now opened **once at simulation start**, and **closed at the end**.
- Replaced `std::endl` with `"\n"` to avoid constant flushing.

##### SINR Logging
- Handled directly in `5g_sim.cc`.
- Declared a global `std::ofstream outSinrFile`.

##### CQI Logging
- Created `cqilog_stream.h` and `cqilog_stream.cc` in `contrib/nr/model/`.
- Declared global `outCqiFile` to be shared across modules.
- Logging now done in `nr-mac-scheduler-cqi-management.cc`.
- File open/close managed centrally in `5g_sim.cc`.

#### Result
- Log file sizes remain manageable.
- Disk I/O reduced significantly.
- Simulation time decreased drastically; now scales better with more UEs or longer durations.


