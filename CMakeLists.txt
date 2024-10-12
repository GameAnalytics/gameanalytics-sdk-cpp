CMAKE_MINIMUM_REQUIRED (VERSION 3.20)

PROJECT (GameAnalytics)

set(GA_SOURCE_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/source")
set(DEPENDENCIES_DIR "${GA_SOURCE_DIR}/dependencies")
set(EXTERNALS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/externals")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(GA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gameanalytics")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeIncludes")

include("create_source_groups_macro")
include("eval_condition_macro")

# --------------------------- Options --------------------------- #
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
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
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)


include_directories(
    # gameanalytics includes
    "${GA_SOURCE_DIR}/gameanalytics"
    "${INCLUDE_DIR}"

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

    "${INCLUDE_DIR}/*.h"
    "${INCLUDE_DIR}/*.cpp"

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

# --------------------------- Detect Platform Automatically --------------------------- #
# Check if the PLATFORM variable was passed in from the command line
if(NOT DEFINED PLATFORM)
    message(STATUS "PLATFORM not set. Detecting platform...")

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(PLATFORM "linux_x64")
        else()
            set(PLATFORM "linux_x86")
        endif()

    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        # macOS
        set(PLATFORM "osx")

    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(PLATFORM "win64")
        elseif(CMAKE_SYSTEM_VERSION MATCHES "10.0")
            # UWP platform
            set(PLATFORM "uwp")
        else()
            set(PLATFORM "win32")
        endif()

    else()
        message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
    endif()

    message(STATUS "Auto-detected platform: ${PLATFORM}")
else()
    message(STATUS "Using user-specified PLATFORM: ${PLATFORM}")
endif()

# --------------------------- Detect Architecture Automatically --------------------------- #

# Print the system architecture
message(STATUS "System architecture: ${CMAKE_SYSTEM_PROCESSOR}")

if(${PLATFORM} STREQUAL "osx")
    # Set archs to be build for osx to both x86_64 and arm64
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")

    if(DEFINED CMAKE_OSX_ARCHITECTURES)
        message(STATUS "Target architectures (CMAKE_OSX_ARCHITECTURES): ${CMAKE_OSX_ARCHITECTURES}")
    else()
        message(STATUS "CMAKE_OSX_ARCHITECTURES is not defined.")
    endif()
else()
    # Detect if it's 32-bit or 64-bit for other systems based on the pointer size
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(STATUS "Target is 64-bit")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(STATUS "Target is 32-bit")
    else()
        message(WARNING "Unknown architecture")
    endif()
endif()

# --------------------------- Settings --------------------------- #

if(${GA_USE_PACKAGE})

    find_package(CURL REQUIRED PATHS ${EXTERNALS_DIR}/curl)
    find_package(OpenSSL REQUIRED PATHS ${EXTERNALS_DIR}/openssl)
    set(LIBS CURL::libcurl)

else()

    add_definitions("-DUSE_OPENSSL -DCURL_STATICLIB -DCRYPTOPP_DISABLE_ASM")

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
    set(PUBLIC_LIBS
        "-framework CoreFoundation"
        "-framework Foundation"
        "-framework CoreServices"
        "-framework SystemConfiguration"
    )

    create_source_groups(MACOS_SOURCES)

elseif(LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGUID_STDLIB -std=c++17")

    if(CLANG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
    
endif()

if(${GA_BUILD_SAMPLE})
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/sample")
endif()

add_library(GameAnalytics ${LIB_TYPE} ${CPP_SOURCES})
target_link_libraries(GameAnalytics PRIVATE ${LIBS} PUBLIC ${PUBLIC_LIBS})

# --------------------------- Google Test Setup --------------------------- #

# Set Project Name
set(UT_PROJECT_NAME "${PROJECT_NAME}UnitTests")

# Add Google Test
set(GTEST_DIR "${EXTERNALS_DIR}/googletest")
add_subdirectory(${GTEST_DIR} ${PROJECT_SOURCE_DIR}/gtest_build)

# Add tests
enable_testing()

########################################
# Test files
########################################

file(GLOB_RECURSE TEST_SRC_FILES "${PROJECT_SOURCE_DIR}/test/*.cpp")

########################################
# Unit Tests
#######################################
add_executable(${UT_PROJECT_NAME} ${TEST_SRC_FILES})

########################################
# Standard linking to gtest and gmock components
########################################
target_link_libraries(${UT_PROJECT_NAME} gtest gtest_main gmock_main)

########################################
# Linking to GA SDK
########################################
target_link_libraries(${UT_PROJECT_NAME} ${PROJECT_NAME})

########################################
add_test(NAME ${UT_PROJECT_NAME} COMMAND GameAnalyticsUnitTests)

# --------------------------- Code Coverage Setup --------------------------- #

if (ENABLE_COVERAGE)
    find_program(GCOV_PATH gcov)
    if (NOT GCOV_PATH)
        message(WARNING "program gcov not found")
    endif()

    find_program(LCOV_PATH lcov)
    if (NOT LCOV_PATH)
        message(WARNING "program lcov not found")
    endif()

    find_program(GENHTML_PATH genhtml)
    if (NOT GENHTML_PATH)
        message(WARNING "program genhtml not found")
    endif()

    if (LCOV_PATH AND GCOV_PATH)

        target_compile_options(
            GameAnalytics
            PRIVATE
                -g -O0 -fprofile-arcs -ftest-coverage
        )

        target_link_libraries(
            GameAnalytics
            PRIVATE
                --coverage
        )

        set(covname cov)

        add_custom_target(cov_data
            # Cleanup lcov
            COMMENT "Resetting code coverage counters to zero."
            ${LCOV_PATH} --directory . --zerocounters

            # Run tests
            COMMAND GameAnalyticsUnitTests

            # Capturing lcov counters and generating report
            COMMAND ${LCOV_PATH} --directory . --capture --output-file ${covname}.info
            COMMAND ${LCOV_PATH} --remove ${covname}.info
                                '${CMAKE_SOURCE_DIR}/source/dependencies/*'
                                '${CMAKE_SOURCE_DIR}/test/*'
                                '/usr/*'
                                '/Applications/Xcode.app/*'
                                --output-file ${covname}.info.cleaned
        )

        if (GENHTML_PATH)
            add_custom_target(cov

                # Cleanup lcov
                ${LCOV_PATH} --directory . --zerocounters

                # Run tests
                COMMAND GameAnalyticsUnitTests

                # Capturing lcov counters and generating report
                COMMAND ${LCOV_PATH} --directory . --capture --output-file ${covname}.info --rc lcov_branch_coverage=1 --rc derive_function_end_line=0
                COMMAND ${LCOV_PATH} --remove ${covname}.info
                                    '${CMAKE_SOURCE_DIR}/source/dependencies/*'
                                    '/usr/*'
                                    --output-file ${covname}.info.cleaned
                                    --rc lcov_branch_coverage=1 
                                    --rc derive_function_end_line=0
                COMMAND ${GENHTML_PATH} -o ${covname} ${covname}.info.cleaned --rc lcov_branch_coverage=1 --rc derive_function_end_line=0
                COMMAND ${CMAKE_COMMAND} -E remove ${covname}.info ${covname}.info.cleaned

                COMMENT "Resetting code coverage counters to zero.\nProcessing code coverage counters and generating report."
            )
        else()
            message(WARNING "unable to generate coverage report: missing genhtml")
        endif()

    else()
        message(WARNING "unable to add coverage targets: missing coverage tools")
    endif()
endif()
