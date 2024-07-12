cmake_minimum_required(VERSION 3.15.0)
project(mp4join VERSION 1.0.0 LANGUAGES CXX)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${CMAKE_CURRENT_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------> Git version
set(GIT_DESC    Unknown)
set(GIT_BRANCH  Unknown)
set(COMMIT_DATE Unknown)
set(COMMIT_HASH Unknown)

site_name(HOST_HOSTNAME)
if(CMAKE_CXX_COMPILER_ID)
  set(COMPILER_NAME ${CMAKE_CXX_COMPILER_ID})
else()
  set(COMPILER_NAME Unknown)
endif()
if(CMAKE_CXX_COMPILER_VERSION)
  set(COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
else()
  set(COMPILER_VERSION Unknown)
endif()

find_package(Git)
if(Git_FOUND)
  # check if we're in a valid git repo
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --is-inside-work-tree OUTPUT_VARIABLE INSIDE_GIT_REPO OUTPUT_STRIP_TRAILING_WHITESPACE)
  if("${INSIDE_GIT_REPO}" STREQUAL "true")
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --long --dirty --broken OUTPUT_VARIABLE GIT_DESC OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%cs OUTPUT_VARIABLE COMMIT_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    message(WARNING "Not in a git repository.")
  endif()
else()
  message(WARNING "Git was not found.")
endif()

set(GENERATED_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(lib/version.cpp.in ${GENERATED_SOURCE_DIR}/version.cpp @ONLY)
# <-------

if(MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif()

set(MP4JOIN_SOURCE_FILES
mp4.hpp mp4.cpp mp4join/mp4join.hpp mp4join.cpp fourcc.hpp
lfs.h binary_file_stream.hpp binary_file_stream.cpp
binary_stream_base.hpp binary_stream_base.cpp endian.h
)
list(TRANSFORM MP4JOIN_SOURCE_FILES PREPEND lib/)

add_library(mp4join STATIC
${MP4JOIN_SOURCE_FILES}
lib/mp4join/version.hpp ${GENERATED_SOURCE_DIR}/version.cpp
)
target_include_directories(mp4join PUBLIC lib)
target_compile_definitions(mp4join PRIVATE MP4JOIN_BUILD_TYPE=\"$<CONFIG>\")

find_package(Threads REQUIRED)
add_executable(mp4join_cli src/mp4join_cli.cpp)
set_target_properties(mp4join_cli PROPERTIES OUTPUT_NAME mp4join)
target_link_libraries(mp4join_cli PRIVATE mp4join Threads::Threads)

# -------> Package
install(TARGETS mp4join
ARCHIVE DESTINATION lib
LIBRARY DESTINATION lib
RUNTIME DESTINATION bin)
install(DIRECTORY lib/mp4join DESTINATION include)
install(TARGETS mp4join_cli DESTINATION bin)

set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${GIT_DESC}_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR})
set(CPACK_GENERATOR "ZIP")
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_STRIP_FILES ON)
include(CPack)
# <-------
