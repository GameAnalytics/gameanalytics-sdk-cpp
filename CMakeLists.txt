CMAKE_MINIMUM_REQUIRED (VERSION 3.20)

PROJECT (GameAnalytics)

set(GA_SOURCE_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/source")
set(DEPENDENCIES_DIR "${GA_SOURCE_DIR}/dependencies")
set(EXTERNALS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/externals")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(GA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gameanalytics")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeIncludes")

include("create_source_groups_macro")
include("eval_condition_macro")

option(GA_SHARED_LIB "Build GA as a shared library" OFF)
option(GA_UWP_BUILD  "Build GA for UWP (if targeting windows)" OFF)
option(GA_BUILD_SAMPLE "Builds the GA Sample app" ON)
option(GA_USE_PACKAGE "Use installed packages for dependencies" OFF)

# set directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${CMAKE_BINARY_DIR}/Release")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${CMAKE_BINARY_DIR}/Release")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

set(CMAKE_CXX_STANDARD 17)

include_directories(
    # gameanalytics includes
    "${GA_SOURCE_DIR}/gameanalytics"

    # depndencies includes
    "${DEPENDENCIES_DIR}"
    "${DEPENDENCIES_DIR}/crossguid"
    "${DEPENDENCIES_DIR}/nlohmann"
    "${DEPENDENCIES_DIR}/stacktrace"
    "${DEPENDENCIES_DIR}/zf_log"
    "${DEPENDENCIES_DIR}/sqlite"
    "${DEPENDENCIES_DIR}/crypto"
    "${DEPENDENCIES_DIR}/miniz"
    "${EXTERNALS_DIR}/curl/include"
    "${EXTERNALS_DIR}/openssl/include"
)

FILE(GLOB_RECURSE CPP_SOURCES
    # Add GameAnalytics Sources
    "${GA_SOURCE_DIR}/gameanalytics/*.h"
    "${GA_SOURCE_DIR}/gameanalytics/*.cpp"

    # Add dependencies
    "${DEPENDENCIES_DIR}/crossguid/*"
    "${DEPENDENCIES_DIR}/nlohmann/*"
    "${DEPENDENCIES_DIR}/stacktrace/*"
    "${DEPENDENCIES_DIR}/zf_log/*"
    "${DEPENDENCIES_DIR}/sqlite/*"
    "${DEPENDENCIES_DIR}/crypto/*"
    "${DEPENDENCIES_DIR}/miniz/*"
    "${DEPENDENCIES_DIR}/stackwalker/*"
)

create_source_groups(CPP_SOURCES)

if(${GA_USE_PACKAGE})
    find_package(CURL REQUIRED PATHS ${EXTERNALS_DIR}/curl)
    find_package(OpenSSL REQUIRED PATHS ${EXTERNALS_DIR}/openssl)
    set(LIBS CURL::libcurl)
else()
    add_definitions("-DUSE_OPENSSL -DCURL_STATICLIB")
    add_definitions("-DCRYPTOPP_DISABLE_ASM")

    link_directories(
        "${EXTERNALS_DIR}/openssl/1.1.1d/libs/${PLATFORM}"
        "${EXTERNALS_DIR}/curl/lib/${PLATFORM}"
    )

    if(WIN32)
        set(LIBS libcurl.lib libeay32.lib ssleay32.lib)
    else()
        set(LIBS libcurl.a libssl.a libcrypto.a)
    endif()

endif()

if(${GA_SHARED_LIB})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGA_SHARED_LIB")
    set(LIB_TYPE SHARED)
else()
    set(LIB_TYPE STATIC)
endif()

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGUID_WINDOWS")
    if(${GA_SHARED_LIB})
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    endif()

    if(${GA_UWP_BUILD})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGA_UWP_BUILD")
    endif()

elseif(APPLE)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGUID_CFUUID")
    FILE(GLOB_RECURSE MACOS_SOURCES "${GA_SOURCE_DIR}/gameanalytics/Platform/*.mm")
    list(APPEND CPP_SOURCES ${MACOS_SOURCES})
    list(APPEND LIBS
        "-framework CoreFoundation"
        "-framework Foundation"
        "-framework CoreServices"
        "-framework systemconfiguration"
    )

    create_source_groups(MACOS_SOURCES)

elseif(LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGUID_LIBUUID")
endif()

if(${GA_BUILD_SAMPLE})
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/sample")
endif()

add_library(GameAnalytics ${LIB_TYPE} ${CPP_SOURCES})
target_link_libraries(GameAnalytics ${LIBS})