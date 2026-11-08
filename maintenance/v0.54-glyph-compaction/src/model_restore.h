#pragma once
#include "assets.h"
#include <map>
#include <optional>
#include <string>

// Load and validate before touching the displayed model. The caller removes
// the persistent custom file only after this succeeds.
inline void restoreDefaultModel(std::map<int,ImportedModel>& models,int id,
                                const std::filesystem::path& path){
 auto replacement=loadModel(path);
 models.insert_or_assign(id,std::move(replacement));
}

struct StartupModel {
 std::optional<ImportedModel> model;
 bool custom=false;
 std::string warning;
};
// Prefer a valid user model without loading a default that would be discarded.
// Empty default paths mean this aircraft uses procedural geometry.
inline StartupModel loadStartupModel(const std::filesystem::path& defaultPath,
                                     const std::filesystem::path& customPath){
 StartupModel result;
 if(std::filesystem::exists(customPath)){
  try{result.model=loadModel(customPath);result.custom=true;return result;}
  catch(const std::runtime_error& error){result.warning=error.what();}
 }
 if(!defaultPath.empty())result.model=loadModel(defaultPath);
 return result;
}
