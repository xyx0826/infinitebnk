cmake_minimum_required(VERSION 3.10.0)
project(infinitebnk VERSION 0.1.0)

set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)
cmake_policy(SET CMP0079 NEW)

add_executable(infinitebnk)

# build libInfinite first
set(INFINITE_EXPORT_OODLE ON)
set(INFINITE_COPY_OODLE_DLL ON)
set(INFINITE_OODLE_DLL oo2core_8_win64.dll)
set(OODLE_LIBRARY "libInfinite/${INFINITE_OODLE_DLL}")
set(FUSE OFF)

# set(CMAKE_FIND_DEBUG_MODE TRUE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/share/jsoncpp")

# set(jsoncpp_ROOT "${CMAKE_CURRENT_LIST_DIR}/3rd_party/jsoncpp")
set(PNG_ROOT "${CMAKE_CURRENT_LIST_DIR}/3rd_party/libpng-1.2.37")
set(ZLIB_ROOT "${CMAKE_CURRENT_LIST_DIR}/3rd_party/zlib-1.2.3")

if(NOT DEFINED LIBINFINITE)
	add_subdirectory(libInfinite/)
else()
	add_library(Infinite UNKNOWN IMPORTED)
	message(STATUS "Using libInfinite from ${LIBINFINITE}")
	set_target_properties(Infinite PROPERTIES IMPORTED_LOCATION ${LIBINFINITE})
endif()

# all of this copying around is needed with linoodle because linoodle expects the dll to be in the cwd
if(${INFINITE_COPY_OODLE_DLL})
	configure_file(libInfinite/${INFINITE_OODLE_DLL} ${INFINITE_OODLE_DLL} COPYONLY)
endif()

set_property(TARGET infinitebnk PROPERTY CXX_STANDARD 20)

target_sources(infinitebnk PRIVATE src/main.cpp)
target_include_directories(infinitebnk PUBLIC src)

# link libInfinite
target_link_libraries(infinitebnk Infinite)

configure_file(libInfinite/3rd_party/detex/LICENSE detex_LICENSE COPYONLY)
