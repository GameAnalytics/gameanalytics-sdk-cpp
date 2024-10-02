#include <string>
#include <iostream>
#include <thread>

#include "GameAnalytics/GameAnalytics.h"

constexpr const char* GAME_KEY   = "bd624ee6f8e6efb32a054f8d7ba11618";
constexpr const char* SECRET_KEY = "7f5c3f682cbd217841efba92e92ffb1b3b6612bc";

struct RemoteConfigs:
    public gameanalytics::IRemoteConfigsListener
{
    void onRemoteConfigsUpdated(std::string const& configs) override
    {
        std::cout << "remote configs = " << configs << '\n';
    }
};

void testCrash()
{
    int* i = nullptr;
    *i = 10;
}

int main(int argc, char** argv)
{
    using namespace gameanalytics;
    
    std::cout << "start\n";
    
    GameAnalytics::setEnabledVerboseLog(true);
    GameAnalytics::setEnabledInfoLog(true);
    
    std::string fields = "{\"my_field\": 1000}";
    
    GameAnalytics::setGlobalCustomEventFields(fields);
    
    GameAnalytics::configureExternalUserId("test_user");
    GameAnalytics::setEnabledErrorReporting(true);

    auto listener = std::make_shared<RemoteConfigs>();
    GameAnalytics::addRemoteConfigsListener(listener);
    
    GameAnalytics::setEnabledManualSessionHandling(true);
    
    GameAnalytics::configureBuild("1.0.0");
    GameAnalytics::configureAvailableResourceCurrencies({"diamonds"});
    GameAnalytics::configureAvailableResourceItemTypes({"diamonds_pack_10000"});
    GameAnalytics::configureAvailableCustomDimensions01({"test"});
    
    GameAnalytics::enableSDKInitEvent();
    GameAnalytics::enableMemoryHistogram();
    GameAnalytics::enableHardwareTracking();
    GameAnalytics::enableFPSHistogram([]() -> float {return 60.f;});
    
    GameAnalytics::setCustomDimension01("test");
    
    using namespace std::chrono_literals;
    
    GameAnalytics::initialize(GAME_KEY, SECRET_KEY);
    
    GameAnalytics::startSession();
    GameAnalytics::addDesignEvent("test_event", 10.);
    GameAnalytics::addBusinessEvent("EUR", 100, "diamonds", "diamond_pack_10000", "dungeon_shop");
    GameAnalytics::addErrorEvent(EGAErrorSeverity::Critical, "failed to load level");
    GameAnalytics::addResourceEvent(EGAResourceFlowType::Source, "diamonds", 100, "diamonds", "big_pack_01");
    GameAnalytics::addProgressionEvent(EGAProgressionStatus::Complete, 100, "volcano", "dungeon", "dragon");
    
    std::this_thread::sleep_for(5000ms);
    //testCrash();
    
    //GameAnalytics::endSession();
    
    std::cout << "test\n";
    
    //std::cin.get();
    
    return 0;
}
