# UWP shell executable for the Xbox port (milestone 1: headless boot with the null renderer).
add_executable(rpcs3_uwp WIN32 uwp_main.cpp)

target_include_directories(rpcs3_uwp PRIVATE
    ${CMAKE_SOURCE_DIR}/rpcs3
    ${CMAKE_SOURCE_DIR})

target_compile_definitions(rpcs3_uwp PRIVATE UNICODE _UNICODE)
target_link_libraries(rpcs3_uwp PRIVATE rpcs3_emu d3d11 dxgi runtimeobject)
target_link_options(rpcs3_uwp PRIVATE -municode -Wl,--stack,8388608)

set_target_properties(rpcs3_uwp PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/uwp)
