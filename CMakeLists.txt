cmake_minimum_required(VERSION 3.15.0)

if(NOT GENERATOR_IS_MULTI_CONFIG)
  # Default to Release if not set
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build.")
endif()

project(mp4join LANGUAGES CXX)

option(BUILD_SHARED_LIBS "Build shared mp4join library" OFF)
option(MP4JOIN_OPTIMIZED_X86_64 "Enable optimized x86-64 builds, targetting x86-64-v3 or equivalent." OFF)

# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
# set(LIBRARY_OUTPUT_PATH    ${CMAKE_CURRENT_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimized x86-64 build
if(MP4JOIN_OPTIMIZED_X86_64 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-march=x86-64-v3" COMPILER_SUPPORTS_MARCH_X86_64_V3)
  add_compile_options($<IF:$<CXX_COMPILER_ID:MSVC>,/arch:AVX2,$<GENEX_EVAL:$<IF:$<BOOL:${COMPILER_SUPPORTS_MARCH_X86_64_V3}>,-march=x86-64-v3,-march=haswell>>>)
endif()

# Do not export symbols by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

# -------> Git version
include(cmake/git-version.cmake)

set(GENERATED_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(lib/version.cpp.in ${GENERATED_SOURCE_DIR}/version.cpp @ONLY)
# <-------

# Enable some warnings
add_compile_options("$<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra;-pedantic>")

# MSVC conformance mode
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/permissive->)
# Enable updated __cplusplus macro for MSVC
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>)

# if(UNIX)
# add_compile_definitions(_POSIX_C_SOURCE=200809L)
# endif()

set(MP4JOIN_SOURCE_FILES
bstream/endian.h bstream/lfs.hpp bstream/binary_stream.hpp bstream/binary_file_stream.hpp bstream/binary_file_stream.cpp
mp4.hpp mp4.cpp mp4join.cpp fourcc.hpp
mp4join/api_export.h mp4join/mp4join.h mp4join/version.h
)
list(TRANSFORM MP4JOIN_SOURCE_FILES PREPEND lib/)

add_library(mp4join
${MP4JOIN_SOURCE_FILES}
${GENERATED_SOURCE_DIR}/version.cpp
)
target_include_directories(mp4join PUBLIC lib)
target_compile_definitions(mp4join PRIVATE CMAKE_MP4JOIN_BUILD_TYPE=\"$<CONFIG>\")
target_compile_definitions(mp4join PRIVATE MP4JOIN_SOURCE)
if(BUILD_SHARED_LIBS)
target_compile_definitions(mp4join PUBLIC MP4JOIN_SHARED_LIB)
endif()

find_package(Threads REQUIRED)
add_executable(mp4join_cli src/mp4join_cli.cpp src/is_tty.hpp src/is_tty.cpp)
set_target_properties(mp4join_cli PROPERTIES OUTPUT_NAME mp4join)
target_link_libraries(mp4join_cli PRIVATE mp4join Threads::Threads)

if(WIN32)
enable_language(RC)
configure_file("cmake/info-cli.rc.in" ${GENERATED_SOURCE_DIR}/info-cli.rc @ONLY)
target_sources(mp4join_cli PRIVATE cmake/icon.rc ${GENERATED_SOURCE_DIR}/info-cli.rc)
if(BUILD_SHARED_LIBS)
# Add metadata rc to dll
endif()
endif()

# -------> Package
install(TARGETS mp4join)
if(BUILD_SHARED_LIBS AND MSVC)
install(FILES $<TARGET_PDB_FILE:mp4join> TYPE BIN OPTIONAL)
endif()
install(DIRECTORY lib/mp4join TYPE INCLUDE)
install(TARGETS mp4join_cli)

set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${GIT_DESC}_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR})
if(NOT BUILD_SHARED_LIBS)
STRING(APPEND CPACK_PACKAGE_FILE_NAME "-staticlib")
endif()
if(CMAKE_BUILD_TYPE)
STRING(APPEND CPACK_PACKAGE_FILE_NAME "-${CMAKE_BUILD_TYPE}")
endif()

set(CPACK_GENERATOR "ZIP")
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_STRIP_FILES ON)
include(CPack)
# <-------
