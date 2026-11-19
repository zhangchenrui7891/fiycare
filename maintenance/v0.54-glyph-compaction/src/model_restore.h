#pragma once
#include "assets.h"
#include <map>
#include <optional>
#include <set>
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
 // Optional custom-path probing can itself throw (permissions, path limits).
 // Keep it inside the same recovery boundary as reading the custom mesh.
 try{
  if(std::filesystem::exists(customPath)){result.model=loadModel(customPath);result.custom=true;return result;}
 }catch(const std::runtime_error& error){result.warning=error.what();}
 if(!defaultPath.empty())result.model=loadModel(defaultPath);
 return result;
}

// Each visited profile is read once, including procedural-only aircraft. A
// failed required mesh is not marked ready, so repair and retry remain possible.
inline std::string ensureAircraftModel(std::map<int,ImportedModel>& models,
                                      std::set<int>& ready,std::set<int>& custom,
                                      int id,const std::filesystem::path& defaultPath,
                                      const std::filesystem::path& customPath){
 if(ready.count(id))return {};
 auto selected=loadStartupModel(defaultPath,customPath);
 if(selected.model)models.insert_or_assign(id,std::move(*selected.model));
 if(selected.custom)custom.insert(id);else custom.erase(id);
 ready.insert(id);
 return selected.warning;
}
