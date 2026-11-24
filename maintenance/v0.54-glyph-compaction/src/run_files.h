#pragma once
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace runfiles {
// Called once at startup. Directory creation claims a session even when
// several launches start within the same second.
inline std::filesystem::path createPracticeSession(const std::filesystem::path& practice,std::time_t now){
 const auto parent=practice/"sessions";
 std::filesystem::create_directories(parent);
 const auto* utc=std::gmtime(&now);
 if(!utc)throw std::runtime_error("Invalid practice session time");
 std::ostringstream name;
 name.imbue(std::locale::classic());
 name<<std::put_time(utc,"%Y-%m-%d_%H-%M-%SZ");
 const auto stamp=name.str();
 for(unsigned index=0;;++index){
  const auto candidate=parent/(index?stamp+"-"+std::to_string(index):stamp);
  if(std::filesystem::create_directory(candidate))return candidate;
 }
}

// Diagnostic outputs never share the live career directory. Benchmark wins
// when both switches are supplied, matching the fixed-scene setup in main.
class RunFiles {
 std::filesystem::path output;
 bool diagnostic;
public:
 RunFiles(const std::filesystem::path& gameFolder,bool benchmark,bool smoke,const char* benchmarkSubfolder="",bool fxaa=true,bool practice=false)
  :output(benchmark?gameFolder/"diagnostics"/"benchmark":
          smoke?gameFolder/"diagnostics"/"smoke":
          practice?gameFolder/"practice"/"a330-final":gameFolder),diagnostic(benchmark||smoke||practice){
  if(benchmark){
   if(!fxaa)output/="no-fxaa";
   if(*benchmarkSubfolder)output/=benchmarkSubfolder;
  }
 }
 const std::filesystem::path& folder()const{return output;}
 bool gameSavesEnabled()const{return !diagnostic;}
};
}
