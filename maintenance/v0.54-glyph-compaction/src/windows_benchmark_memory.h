#pragma once
#include "benchmark.h"
#include <windows.h>
#include <psapi.h>

namespace timing {
inline std::optional<ProcessWorkingSet> readProcessWorkingSet(){
 PROCESS_MEMORY_COUNTERS counters{};
 counters.cb=static_cast<DWORD>(sizeof(counters));
 if(!GetProcessMemoryInfo(GetCurrentProcess(),&counters,counters.cb))return std::nullopt;
 return ProcessWorkingSet{counters.WorkingSetSize,counters.PeakWorkingSetSize};
}
}
