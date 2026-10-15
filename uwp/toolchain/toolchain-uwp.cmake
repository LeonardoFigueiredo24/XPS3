# Cross-compile RPCS3 for x64 UWP (Xbox Series X|S Dev Mode) from Linux.
# Uses Zig's bundled clang + libc++ + mingw-w64 (UCRT) as the toolchain: pip install ziglang
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_C_COMPILER ${CMAKE_CURRENT_LIST_DIR}/zig-cc)
set(CMAKE_CXX_COMPILER ${CMAKE_CURRENT_LIST_DIR}/zig-c++)
set(CMAKE_AR ${CMAKE_CURRENT_LIST_DIR}/zig-ar)
set(CMAKE_RANLIB ${CMAKE_CURRENT_LIST_DIR}/zig-ranlib)
find_program(CMAKE_RC_COMPILER NAMES llvm-windres llvm-windres-18 llvm-windres-19 llvm-windres-20)
set(CMAKE_RC_FLAGS_INIT "--target=pe-x86-64")
# libpng generates pnglibconf with the C preprocessor and needs the bundled zlib headers.
set(CMAKE_C_FLAGS_INIT "-I${CMAKE_CURRENT_LIST_DIR}/../../3rdparty/zlib/zlib")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
