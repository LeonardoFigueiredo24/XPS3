# Qt-less headless runner for PC: boots PS3 ELFs one after another with the Null renderer and
# prints their TTY output with timings. Used to check the bench/test ELFs before Xbox runs.
add_executable(rpcs3_bench_runner
    bench_runner.cpp
    ${CMAKE_SOURCE_DIR}/uwp/app/uwp_stubs.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/rpcs3_version.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/pad_thread.cpp
    ${CMAKE_SOURCE_DIR}/rpcs3/Input/product_info.cpp)
target_include_directories(rpcs3_bench_runner PRIVATE ${CMAKE_SOURCE_DIR}/rpcs3 ${CMAKE_SOURCE_DIR})
target_link_libraries(rpcs3_bench_runner PRIVATE rpcs3_emu 3rdparty::zlib 3rdparty::stblib)
