# libPNG
# Select the version of libpng to use, default is builtin
if (NOT USE_SYSTEM_LIBPNG)
	# We use libpng's static library and don't need to build the shared library and run the tests
	set(PNG_SHARED OFF CACHE BOOL "Build shared lib")
	set(PNG_TESTS OFF CACHE BOOL "Build libpng tests")
	set(SKIP_INSTALL_ALL ON)
	if (RPCS3_UWP)
		# The repository can be checked out on Windows while the UWP cross-build runs in Linux.
		# libpng feeds several source files directly to awk, and CRLF corrupts its DFA tokens.
		# Work from a normalized build-tree copy so the pinned submodule checkout stays untouched.
		set(XPS3_LIBPNG_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libpng-source")
		file(REMOVE_RECURSE "${XPS3_LIBPNG_SOURCE_DIR}")
		file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/libpng/" DESTINATION "${XPS3_LIBPNG_SOURCE_DIR}")
		file(GLOB_RECURSE XPS3_LIBPNG_TEXT_INPUTS
			"${XPS3_LIBPNG_SOURCE_DIR}/scripts/*.awk"
			"${XPS3_LIBPNG_SOURCE_DIR}/scripts/*.dfa"
			"${XPS3_LIBPNG_SOURCE_DIR}/scripts/*.dfn"
			"${XPS3_LIBPNG_SOURCE_DIR}/scripts/*.cmake"
			"${XPS3_LIBPNG_SOURCE_DIR}/pngconf.h")
		foreach(XPS3_LIBPNG_TEXT_INPUT IN LISTS XPS3_LIBPNG_TEXT_INPUTS)
			file(READ "${XPS3_LIBPNG_TEXT_INPUT}" XPS3_LIBPNG_TEXT)
			string(REPLACE "\r\n" "\n" XPS3_LIBPNG_TEXT "${XPS3_LIBPNG_TEXT}")
			file(WRITE "${XPS3_LIBPNG_TEXT_INPUT}" "${XPS3_LIBPNG_TEXT}")
		endforeach()
		add_subdirectory("${XPS3_LIBPNG_SOURCE_DIR}" libpng EXCLUDE_FROM_ALL)
	else()
		add_subdirectory(libpng EXCLUDE_FROM_ALL)
	endif()
	target_include_directories(png_static SYSTEM INTERFACE "${libpng_BINARY_DIR}" "${libpng_SOURCE_DIR}")

	set(LIBPNG_TARGET png_static PARENT_SCOPE)
else()
	find_package(PNG REQUIRED)

	add_library(3rdparty_system_libpng INTERFACE)
	target_include_directories(3rdparty_system_libpng SYSTEM INTERFACE ${PNG_INCLUDE_DIR})
	target_link_libraries(3rdparty_system_libpng INTERFACE ${PNG_LIBRARY})
	target_compile_definitions(3rdparty_system_libpng INTERFACE ${PNG_DEFINITIONS})

	set(LIBPNG_TARGET 3rdparty_system_libpng PARENT_SCOPE)
endif()
