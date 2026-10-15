# UWP shell executable for the Xbox port (milestone 1: headless boot with the null renderer).
add_executable(rpcs3_uwp WIN32
    uwp_main.cpp
    uwp_stubs.cpp
    uwp_sysstubs.c
    ${CMAKE_SOURCE_DIR}/rpcs3/rpcs3_version.cpp
    # Qt-free input code (upstream builds these in the Qt UI target).
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ds3_pad_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ds4_pad_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/dualsense_pad_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/hid_instance.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/hid_pad_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/mm_joystick_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/pad_thread.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/product_info.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ps_move_calibration.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ps_move_config.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ps_move_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/ps_move_tracker.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/raw_mouse_config.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/skateboard_pad_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/wiimote_handler.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/xinput_pad_handler.cpp)

target_include_directories(rpcs3_uwp PRIVATE
    ${CMAKE_SOURCE_DIR}/rpcs3
    ${CMAKE_SOURCE_DIR})

target_compile_definitions(rpcs3_uwp PRIVATE UNICODE _UNICODE)
target_link_libraries(rpcs3_uwp PRIVATE
    rpcs3_emu 3rdparty::hidapi 3rdparty::libusb 3rdparty::stblib 3rdparty::opencv 3rdparty::fusion
    d3d11 dxgi winmm
    # WinRT activation/strings and the UWP *FromApp APIs (import libs from mkimplibs.sh)
    winrt_activation winrt_string uwp_memory_6 uwp_memory_5 uwp_memory_3 uwp_memory_1 uwp_file_fromapp)
target_link_options(rpcs3_uwp PRIVATE -municode -Wl,--stack,8388608)

set_target_properties(rpcs3_uwp PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/uwp)
