cmake_minimum_required(VERSION 3.13)
project(cornkidz_n64_portable C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
add_library(ck64_gameflow STATIC src/ck64_gameflow.c src/ck64_save.c src/ck64_gameflow_save.c src/ck64_music.c)
target_include_directories(ck64_gameflow PUBLIC include)
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ck64_gameflow PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
enable_testing()
add_executable(test_gameflow tests/test_gameflow.c)
target_link_libraries(test_gameflow PRIVATE ck64_gameflow)
add_test(NAME gameflow_host_tests COMMAND test_gameflow)

add_executable(test_save tests/test_save.c)
target_link_libraries(test_save PRIVATE ck64_gameflow)
add_test(NAME save_format_host_tests COMMAND test_save)

add_executable(test_music tests/test_music.c)
target_link_libraries(test_music PRIVATE ck64_gameflow)
add_test(NAME musicfade_host_tests COMMAND test_music)

# Phase 5: integer frame scheduler, source-grounded runtime integration.
target_sources(ck64_gameflow PRIVATE src/ck64_clock.c src/ck64_runtime.c)
add_executable(test_clock_runtime tests/test_clock_runtime.c)
target_link_libraries(test_clock_runtime PRIVATE ck64_gameflow)
add_test(NAME clock_runtime_host_tests COMMAND test_clock_runtime)

# Phase 8: independent 3D navigation experiment. NOT PlayerCtrl reverse engineering.
target_sources(ck64_gameflow PRIVATE src/ck64_world3d.c)
target_link_libraries(ck64_gameflow PUBLIC m)
add_executable(test_world3d tests/test_world3d.c)
target_link_libraries(test_world3d PRIVATE ck64_gameflow)
add_test(NAME wireframe_navigation_prototype COMMAND test_world3d)

# Phase 12: host-compiled REAL N64 wireframe source using mock graphics calls.
# Tests the actual number of world-space lines for each grid density.
add_executable(test_wireframe_modes ../n64/wireframe.c ../n64/tests/test_wireframe_modes.c)
target_include_directories(test_wireframe_modes PRIVATE ../n64/tests/stubs ../n64)
target_link_libraries(test_wireframe_modes PRIVATE ck64_gameflow)
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_wireframe_modes PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME grid_mode_line_budget COMMAND test_wireframe_modes)

# Phase 13: timer stage math with independent C11 tests.
target_sources(ck64_gameflow PRIVATE src/ck64_perf.c)
add_executable(test_perf tests/test_perf.c)
target_link_libraries(test_perf PRIVATE ck64_gameflow)
add_test(NAME display_stage_timings COMMAND test_perf)

# Phase 14: source-grounded PlayerCtrl function subset, NOT wired into gameplay.
target_sources(ck64_gameflow PRIVATE src/ck64_playercore.c)
add_executable(test_playercore tests/test_playercore.c)
target_link_libraries(test_playercore PRIVATE ck64_gameflow)
add_test(NAME playerctrl_original_il_subset COMMAND test_playercore)

# Phase 15: source-grounded MovPl C11 math, not wired to placeholder gameplay.
target_sources(ck64_gameflow PRIVATE src/ck64_player_motion.c)
add_executable(test_player_motion tests/test_player_motion.c)
target_link_libraries(test_player_motion PRIVATE ck64_gameflow)
add_test(NAME playerctrl_movpl_source_math COMMAND test_player_motion)

# Phase 16: source-grounded PlayerCtrl Run prelude, not wired into gameplay.
target_sources(ck64_gameflow PRIVATE src/ck64_player_run.c)
add_executable(test_player_run tests/test_player_run.c)
target_link_libraries(test_player_run PRIVATE ck64_gameflow)
add_test(NAME playerctrl_run_prelude COMMAND test_player_run)

# Phase 17: source-derived original state-0 jump/movement-plan.
target_sources(ck64_gameflow PRIVATE src/ck64_player_state0.c)
add_executable(test_player_state0 tests/test_player_state0.c)
target_link_libraries(test_player_state0 PRIVATE ck64_gameflow)
add_test(NAME playerctrl_state0_jump_source COMMAND test_player_state0)

# Phase 18: source-derived PlayerCtrl.Run footer and contact history.
target_sources(ck64_gameflow PRIVATE src/ck64_player_footer.c)
add_executable(test_player_footer tests/test_player_footer.c)
target_link_libraries(test_player_footer PRIVATE ck64_gameflow)
add_test(NAME playerctrl_run_floor_history COMMAND test_player_footer)

# Phase 19: actual movement of a TEST avatar via source-grounded modules.
target_sources(ck64_gameflow PRIVATE src/ck64_source_lab.c)
add_executable(test_source_lab tests/test_source_lab.c)
target_link_libraries(test_source_lab PRIVATE ck64_gameflow)
add_test(NAME source_driven_wireframe_lab COMMAND test_source_lab)

# Phase 22: experimental N64-oriented big-endian CKM1 geometry decoder.
target_sources(ck64_gameflow PRIVATE src/ck64_mesh_format.c)
add_executable(test_mesh_format tests/test_mesh_format.c)
target_link_libraries(test_mesh_format PRIVATE ck64_gameflow)
add_test(NAME ckm1_big_endian_mesh_format COMMAND test_mesh_format)
