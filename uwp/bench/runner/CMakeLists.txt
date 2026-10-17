# Qt-less headless runner for PC: boots PS3 ELFs one after another with the Null renderer and
# prints their TTY output with timings. Used to check the bench/test ELFs before Xbox runs.
add_executable(rpcs3_bench_runner
    bench_runner.cpp
    ${CMAKE_SOURCE_DIR}/uwp/app/uwp_stubs.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/rpcs3_version.cpp
    # Qt-free input code, as in the UWP app (uwp/app/CMakeLists.txt).
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ds3_pad_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ds4_pad_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/dualsense_pad_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/hid_instance.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/hid_pad_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/pad_thread.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/product_info.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ps_move_calibration.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ps_move_config.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ps_move_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ps_move_tracker.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/raw_mouse_config.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/skateboard_pad_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/wiimote_handler.cpp)
target_include_directories(rpcs3_bench_runner PRIVATE ${CMAKE_SOURCE_DIR}/rpcs3 ${CMAKE_SOURCE_DIR})
target_link_libraries(rpcs3_bench_runner PRIVATE rpcs3_emu 3rdparty::hidapi 3rdparty::zlib 3rdparty::libusb 3rdparty::stblib 3rdparty::opencv 3rdparty::fusion)
# pad_thread.cpp pulls in the Qt keyboard handler unless built Qt-less like the UWP app.
set_source_files_properties(${CMAKE_SOURCE_DIR}/rpcs3/Input/pad_thread.cpp PROPERTIES COMPILE_DEFINITIONS RPCS3_UWP)
