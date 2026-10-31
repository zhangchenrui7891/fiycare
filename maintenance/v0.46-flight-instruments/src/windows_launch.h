#pragma once
#include "launch_options.h"
#include <windows.h>
#include <shellapi.h>
#include <stdexcept>

namespace launch {
inline Options fromWindowsCommandLine(const wchar_t* fullCommandLine){
 int count=0;
 auto raw=CommandLineToArgvW(fullCommandLine,&count);
 if(!raw)throw std::runtime_error("Windows command-line parsing failed");
 struct Release {LPWSTR* value;~Release(){LocalFree(value);}} release{raw};
 std::vector<std::wstring> values;
 // argv[0] is the executable path, even if it contains flag-like text.
 for(int i=1;i<count;++i)values.emplace_back(raw[i]);
 return Options(std::move(values));
}
inline Options currentOptions(){return fromWindowsCommandLine(GetCommandLineW());}
}
