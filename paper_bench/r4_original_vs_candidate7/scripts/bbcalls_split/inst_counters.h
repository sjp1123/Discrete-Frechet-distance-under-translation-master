#pragma once
// scratch instrumentation (not part of the repo): per-query counters, reset by the harness
inline long long* inst_cnt() { static long long c[16] = {0}; return c; }
inline void inst_reset() { for (int i = 0; i < 16; ++i) inst_cnt()[i] = 0; }
#define INST_BB() ((long long)MEASUREMENT::getEntry<MEASUREMENT::MeasurementTool::CounterEntry>(EXP::BBCALLS_COUNTER).value)
