# set directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${CMAKE_BINARY_DIR}/Release")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${CMAKE_BINARY_DIR}/Release")

set(CMAKE_CXX_STANDARD 17)

if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

    # Match the library's standard-library choice so the sample's ABI lines up
    # with libGameAnalytics when built with clang.
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
endif()

set(SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp")

add_executable(GASampleShared ${SOURCES})
target_link_libraries(GASampleShared GameAnalytics)
