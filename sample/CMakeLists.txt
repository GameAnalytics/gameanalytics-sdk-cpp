
# set directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${CMAKE_BINARY_DIR}/Release")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${CMAKE_BINARY_DIR}/Release")

set(CMAKE_CXX_STANDARD 17)

if(LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

    if(CLANG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
    
endif()

set(GA_ROOT_DIR "..")

set(GA_SOURCE_DIR   "${GA_ROOT_DIR}/source")
set(DEPENDENCIES_DIR "${GA_ROOT_DIR}/source/dependencies")
set(LIB_DIR "${GA_ROOT_DIR}/libs")
set(GA_DIR "${GA_ROOT_DIR}/gameanalytics")

include("create_source_groups_macro")
include("eval_condition_macro")

include_directories(
    "${GA_SOURCE_DIR}/gameanalytics"
    "${DEPENDENCIES_DIR}"
    "${DEPENDENCIES_DIR}/crossguid"
    "${DEPENDENCIES_DIR}/nlohmann"
    "${DEPENDENCIES_DIR}/stacktrace"
    "${DEPENDENCIES_DIR}/zf_log"
    "${DEPENDENCIES_DIR}/sqlite"
    "${DEPENDENCIES_DIR}/crypto"
    "${DEPENDENCIES_DIR}/miniz"
)

set(SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp")

add_executable(GASample ${SOURCES})
target_link_libraries(GASample GameAnalytics)

