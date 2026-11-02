#pragma once
#include "assets.h"
#include <map>

// Load and validate before touching the displayed model. The caller removes
// the persistent custom file only after this succeeds.
inline void restoreDefaultModel(std::map<int,ImportedModel>& models,int id,
                                const std::filesystem::path& path){
 auto replacement=loadModel(path);
 models.insert_or_assign(id,std::move(replacement));
}
