#-------- Version & Build info
# NOTE: version.cpp is only updated on CMake configure.
# To update every time we build, use a custom target that runs the following as a .cmake script.

set(GIT_DESC    Unknown)
set(GIT_BRANCH  Unknown)
set(GIT_TAG     Unknown)
set(COMMIT_DATE Unknown)  # date of last commit
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
        execute_process(COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 OUTPUT_VARIABLE GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%cs OUTPUT_VARIABLE COMMIT_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        message(WARNING "Not in a git repository.")
    endif()
else()
    message(WARNING "Git was not found.")
endif()

# parse version into major, minor, patch
string(REGEX MATCH "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)" git_version_match "${GIT_TAG}")

if(git_version_match)
    set(GIT_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(GIT_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(GIT_VERSION_PATCH "${CMAKE_MATCH_3}")
    set(GIT_VERSION_BUILD "0")
else()
    set(GIT_VERSION_MAJOR Unknown)
    set(GIT_VERSION_MINOR Unknown)
    set(GIT_VERSION_PATCH Unknown)
    set(GIT_VERSION_BUILD Unknown)
endif()
