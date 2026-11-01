#pragma once
#include <filesystem>

namespace runfiles {
// Diagnostic outputs never share the live career directory. Benchmark wins
// when both switches are supplied, matching the fixed-scene setup in main.
class RunFiles {
 std::filesystem::path output;
 bool diagnostic;
public:
 RunFiles(const std::filesystem::path& gameFolder,bool benchmark,bool smoke)
  :output(benchmark?gameFolder/"diagnostics"/"benchmark":
          smoke?gameFolder/"diagnostics"/"smoke":gameFolder),diagnostic(benchmark||smoke){}
 const std::filesystem::path& folder()const{return output;}
 bool gameSavesEnabled()const{return !diagnostic;}
};
}
