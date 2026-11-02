cmake_minimum_required(VERSION 3.20)

project(GASteamSample CXX)

# Standalone sample: configure this folder directly, it pulls in the SDK from the parent folder.
#   cmake -B build -DSTEAMWORKS_SDK_DIR=/path/to/steamworks_sdk/sdk
#   cmake --build build --target GASteamSample

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(STEAMWORKS_SDK_DIR "" CACHE PATH "Steamworks SDK folder (the one containing public/ and redistributable_bin/)")
set(STEAM_APP_ID "480" CACHE STRING "App ID written to steam_appid.txt (480 = Spacewar, Valve's public test app)")

if(NOT EXISTS "${STEAMWORKS_SDK_DIR}/public/steam/steam_api.h")
    message(FATAL_ERROR "Set STEAMWORKS_SDK_DIR to the Steamworks SDK 'sdk' folder (expected ${STEAMWORKS_SDK_DIR}/public/steam/steam_api.h)")
endif()

# --------------------------- Steamworks --------------------------- #

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(STEAM_BIN_DIR "${STEAMWORKS_SDK_DIR}/redistributable_bin/win64")
        set(STEAM_RUNTIME "${STEAM_BIN_DIR}/steam_api64.dll")
        set(STEAM_IMPLIB  "${STEAM_BIN_DIR}/steam_api64.lib")
    else()
        set(STEAM_BIN_DIR "${STEAMWORKS_SDK_DIR}/redistributable_bin")
        set(STEAM_RUNTIME "${STEAM_BIN_DIR}/steam_api.dll")
        set(STEAM_IMPLIB  "${STEAM_BIN_DIR}/steam_api.lib")
    endif()
elseif(APPLE)
    set(STEAM_RUNTIME "${STEAMWORKS_SDK_DIR}/redistributable_bin/osx/libsteam_api.dylib")
else()
    set(STEAM_RUNTIME "${STEAMWORKS_SDK_DIR}/redistributable_bin/linux64/libsteam_api.so")
endif()

add_library(steam_api SHARED IMPORTED)
set_target_properties(steam_api PROPERTIES
    IMPORTED_LOCATION "${STEAM_RUNTIME}"
    INTERFACE_INCLUDE_DIRECTORIES "${STEAMWORKS_SDK_DIR}/public")
if(WIN32)
    set_target_properties(steam_api PROPERTIES IMPORTED_IMPLIB "${STEAM_IMPLIB}")
endif()

# --------------------------- GameAnalytics with the Steam storefront --------------------------- #

set(GA_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(GA_BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory("${GA_ROOT_DIR}" gameanalytics)

# compiles StoreFront/GASteamFront.cpp into the library and links it against the game's Steamworks SDK
target_compile_definitions(GameAnalytics PUBLIC GA_STORE_STEAM)
target_link_libraries(GameAnalytics PUBLIC steam_api)

# --------------------------- Sample --------------------------- #

add_executable(GASteamSample "${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp")
target_link_libraries(GASteamSample PRIVATE GameAnalytics)

# the SDK does not export its include directories, so list them like sample/CMakeLists.txt does
target_include_directories(GASteamSample PRIVATE
    "${GA_ROOT_DIR}/include"
    "${GA_ROOT_DIR}/source/gameanalytics"
    "${GA_ROOT_DIR}/source/dependencies"
    "${GA_ROOT_DIR}/source/dependencies/nlohmann"
    "${GA_ROOT_DIR}/source/dependencies/zf_log"
)

# Linux: find libsteam_api.so next to the executable (macOS: the dylib's install name is already @loader_path)
set_target_properties(GASteamSample PROPERTIES BUILD_RPATH "$ORIGIN")

# the Steam runtime and steam_appid.txt must sit next to the executable
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/steam_appid.txt" "${STEAM_APP_ID}")
add_custom_command(TARGET GASteamSample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${STEAM_RUNTIME}" "$<TARGET_FILE_DIR:GASteamSample>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/steam_appid.txt" "$<TARGET_FILE_DIR:GASteamSample>"
)
