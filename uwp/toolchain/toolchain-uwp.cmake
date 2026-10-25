# Cross-compile RPCS3 for x64 UWP (Xbox Series X|S Dev Mode) from Linux.
# Uses Zig's bundled clang + libc++ + mingw-w64 (UCRT) as the toolchain: pip install ziglang
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_C_COMPILER ${CMAKE_CURRENT_LIST_DIR}/zig-cc)
set(CMAKE_CXX_COMPILER ${CMAKE_CURRENT_LIST_DIR}/zig-c++)
set(CMAKE_AR ${CMAKE_CURRENT_LIST_DIR}/zig-ar)
set(CMAKE_RANLIB ${CMAKE_CURRENT_LIST_DIR}/zig-ranlib)
find_program(CMAKE_RC_COMPILER NAMES llvm-windres llvm-windres-18 llvm-windres-19 llvm-windres-20)
execute_process(COMMAND python3 -c "import ziglang, os; print(os.path.dirname(ziglang.__file__))"
    OUTPUT_VARIABLE ZIG_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
# CMake evaluates a toolchain file again inside every TryCompile directory. Use one cache for the
# entire configure so the expensive case-insensitive header scan and import-library generation are
# not repeated for each compiler probe.
if(DEFINED ENV{XPS3_TOOLCHAIN_CACHE} AND NOT "$ENV{XPS3_TOOLCHAIN_CACHE}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{XPS3_TOOLCHAIN_CACHE}" UWP_TOOLCHAIN_CACHE)
else()
    set(UWP_TOOLCHAIN_CACHE ${CMAKE_BINARY_DIR}/xps3-toolchain-cache)
endif()
# Case-insensitive include shims for mixed-case Windows header names.
set(UWP_CASE_INCLUDES ${UWP_TOOLCHAIN_CACHE}/uwp-case-includes)
if(NOT EXISTS ${UWP_CASE_INCLUDES})
    execute_process(COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/mkcaselinks.py
        ${ZIG_DIR}/lib/libc/include/any-windows-any ${CMAKE_CURRENT_LIST_DIR}/../.. ${UWP_CASE_INCLUDES})
endif()
# Import libraries Zig can't resolve on its own (see mkimplibs.sh).
set(UWP_IMPLIBS ${UWP_TOOLCHAIN_CACHE}/uwp-implibs)
if(NOT EXISTS ${UWP_IMPLIBS})
    execute_process(COMMAND sh ${CMAKE_CURRENT_LIST_DIR}/mkimplibs.sh ${ZIG_DIR} ${UWP_IMPLIBS})
endif()
set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${UWP_IMPLIBS}")
set(CMAKE_RC_FLAGS_INIT "--target=pe-x86-64 -I${ZIG_DIR}/lib/libc/include/any-windows-any -I${UWP_CASE_INCLUDES}")
# libpng generates pnglibconf with the C preprocessor and needs the bundled zlib headers.
set(CMAKE_C_FLAGS_INIT "-I${CMAKE_CURRENT_LIST_DIR}/../../3rdparty/zlib/zlib -idirafter ${UWP_CASE_INCLUDES}")
set(CMAKE_CXX_FLAGS_INIT "-idirafter ${UWP_CASE_INCLUDES}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
