#include <iostream>
#include <thread>
#include <chrono>

#include "GameAnalyticsExtern.h"

constexpr const char* GAME_KEY   = "INSERT_GAME_KEY";
constexpr const char* SECRET_KEY = "INSERT_SECRET_KEY";

void testCrash()
{
    int* i = nullptr;
    *i = 10;
}

int main(int argc, char** argv)
{
    std::cout << "start\n";

    gameAnalytics_setEnabledVerboseLog(EGAEnabled);
    gameAnalytics_setEnabledInfoLog(EGAEnabled);

    gameAnalytics_setGlobalCustomEventFields("{\"my_field\": 1000}");

    gameAnalytics_configureExternalUserId("user_supernew657566453442");
    gameAnalytics_setEnabledErrorReporting(EGAEnabled);
    gameAnalytics_setEnabledManualSessionHandling(EGAEnabled);

    gameAnalytics_configureBuild("1.0.0");

    const char* resourceCurrencies[] = {"diamonds"};
    gameAnalytics_configureAvailableResourceCurrencies(resourceCurrencies, 1);

    const char* resourceItemTypes[] = {"diamonds_pack_10000"};
    gameAnalytics_configureAvailableResourceItemTypes(resourceItemTypes, 1);

    const char* customDimensions01[] = {"test"};
    gameAnalytics_configureAvailableCustomDimensions01(customDimensions01, 1);

    gameAnalytics_enableSDKInitEvent(EGAEnabled);
    gameAnalytics_enableMemoryHistogram(EGAEnabled);
    gameAnalytics_enableHardwareTracking(EGAEnabled);
    gameAnalytics_enableFPSHistogram([]() -> float { return 60.f; }, EGAEnabled);

    gameAnalytics_setCustomDimension01("test");

    using namespace std::chrono_literals;

    gameAnalytics_initialize(GAME_KEY, SECRET_KEY);

    gameAnalytics_startSession();
    gameAnalytics_addDesignEventWithValue("test_event", 10.0, "", EGADisabled);
    gameAnalytics_addBusinessEvent("EUR", 100, "diamonds", "diamond_pack_10000", "dungeon_shop", "", EGADisabled);
    gameAnalytics_addErrorEvent(EGACritical, "failed to load level", "", EGADisabled);
    gameAnalytics_addResourceEvent(EGASource, "diamonds", 100, "diamonds", "big_pack_01", "", EGADisabled);
    gameAnalytics_addProgressionEventWithScore(EGAComplete, "volcano", "dungeon", "dragon", 100, "", EGADisabled);

    std::this_thread::sleep_for(5000ms);
    //testCrash();

    //gameAnalytics_endSession();

    std::cout << "test\n";

    //std::cin.get();

    return 0;
}
