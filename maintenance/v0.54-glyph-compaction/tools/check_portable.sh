#!/usr/bin/env bash
# Rewritten for v0.42 recovery. The original v0.41 script could not be recovered.
set -euo pipefail
task_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
task_build="${1:-$task_root/out/portable}"
mkdir -p -- "$task_build"
for task_test in alert_focus_tests map_clip_tests instrument_format_tests glyph_runs_tests tests aero_avionics_tests engineering_tests stability_tests terrain_stream_tests frame_geometry_tests shader_cache_tests keyboard_state_tests keyboard_event_tests cockpit_input_tests cockpit_solids_tests approach_tests navigation_tests navigation_track_tests benchmark_tests performance_window_tests launch_options_tests flight_instrument_tests autopilot_feedback_tests asset_length_tests scenery_tests; do
    g++ -std=c++17 -O2 -pthread -Wall -Wextra -Werror -pedantic \
        "$task_root/$task_test.cpp" -o "$task_build/$task_test"
    case "$task_test" in
        tests|frame_geometry_tests) "$task_build/$task_test" "$task_root/assets" ;;
        *) "$task_build/$task_test" ;;
    esac
done
g++ -std=c++17 -O2 -pthread -Wall -Wextra -Werror -pedantic \
    "$task_root/flight_lab.cpp" -o "$task_build/flight_lab"
"$task_build/flight_lab" sweep "$task_build/lab/sweep"
"$task_build/flight_lab" scenario --surface --aircraft 3 --seconds 90 \
    --fail-left 15 --output "$task_build/lab/engine_out"
for task_script in check_glass check_clean_overlay check_scenery_integration check_launch_integration check_rebuilt_text check_rebuilt_storage check_frame_upload check_shader_cache_integration check_keyboard_integration check_approach_integration check_menus; do
    python3 "$task_root/tools/$task_script.py"
done
