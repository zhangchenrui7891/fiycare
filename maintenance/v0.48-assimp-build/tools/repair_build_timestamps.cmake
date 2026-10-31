# ZIP extraction can preserve source timestamps ahead of the Windows clock.
# Ninja then continually rebuilds its own manifest. Never change file contents,
# the OS clock, generated outputs, or files outside this source tree.
function(fcair_check_build_timestamps root repair)
  get_filename_component(root "${root}" REALPATH)
  file(GLOB inputs LIST_DIRECTORIES false
    "${root}/CMakeLists.txt" "${root}/CMakePresets.json" "${root}/*.cpp")
  foreach(subdir src tools third_party)
    file(GLOB_RECURSE more LIST_DIRECTORIES false
      "${root}/${subdir}/CMakeLists.txt" "${root}/${subdir}/*.cmake"
      "${root}/${subdir}/*.in" "${root}/${subdir}/*.cpp"
      "${root}/${subdir}/*.cc" "${root}/${subdir}/*.cxx"
      "${root}/${subdir}/*.c" "${root}/${subdir}/*.h"
      "${root}/${subdir}/*.hpp" "${root}/${subdir}/*.inl")
    list(APPEND inputs ${more})
  endforeach()
  string(TIMESTAMP now "%s" UTC)
  set(count 0)
  set(examples "")
  foreach(input IN LISTS inputs)
    if(IS_SYMLINK "${input}")
      continue()
    endif()
    get_filename_component(real_input "${input}" REALPATH)
    string(FIND "${real_input}" "${root}/" inside)
    if(NOT inside EQUAL 0)
      continue()
    endif()
    file(TIMESTAMP "${input}" stamp "%s" UTC)
    if(stamp GREATER now)
      math(EXPR count "${count} + 1")
      if(count LESS_EQUAL 5)
        file(RELATIVE_PATH relative "${root}" "${input}")
        string(APPEND examples "\n  ${relative}")
      endif()
      if(repair)
        file(TOUCH_NOCREATE "${input}")
      endif()
    endif()
  endforeach()
  if(count GREATER 0 AND NOT repair)
    message(FATAL_ERROR
      "FCAIR_FUTURE_TIMESTAMPS: ${count} build inputs are newer than the system clock.${examples}\n"
      "This can cause Ninja's 'manifest build.ninja still dirty after 100 tries'.\n"
      "Check Windows date/time, close the current build, then run repair_build.bat.\n"
      "Or run: cmake -DFCAIR_REPAIR=ON -P tools/repair_build_timestamps.cmake\n"
      "Reconfigure and build again. Do not disable CMake regeneration.")
  elseif(repair)
    message(STATUS "FCAIR_TIMESTAMP_REPAIR: normalized ${count} future build-input timestamps; file contents unchanged")
  endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
  if(NOT DEFINED FCAIR_ROOT)
    get_filename_component(FCAIR_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
  endif()
  fcair_check_build_timestamps("${FCAIR_ROOT}" "${FCAIR_REPAIR}")
endif()
