# Always stage on an explicit application build, even if no C++ file changed.
# CMake >= 3.22 sets CMP0112 NEW: TARGET_FILE_DIR adds no reverse dependency.
function(fcair_stage_runtime_assets target asset_source)
 if(FCAIR_COPY_RUNTIME_ASSETS)
  add_custom_target(${target}_runtime_assets
   COMMAND "${CMAKE_COMMAND}" -E copy_directory
     "${asset_source}" "$<TARGET_FILE_DIR:${target}>/assets"
   COMMENT "Refreshing runtime assets for ${target}"
   VERBATIM)
  add_dependencies(${target} ${target}_runtime_assets)
 else()
  message(STATUS "Runtime asset copy disabled: executable requires separately supplied assets before launch")
 endif()
endfunction()
