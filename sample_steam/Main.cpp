// Minimal "game" showing the Steam storefront.
//
// The game does what every Steam game already does (SteamAPI_Init, SteamAPI_RunCallbacks every frame, SteamAPI_Shutdown)
// plus one extra line, configureStoreFront<GASteamFront>(). Everything else in this file sends a few game events once
// and pokes Steam so the storefront has something to report:
//   - the SteamID becomes the user id, Steam Deck is reported as the device
//   - unlocking an achievement sends store:achievement:<name>
//   - creating a lobby sends store:social:lobby_joined
// Run it with the Steam client open and logged in. App 480 (Spacewar) is Valve's public test app.

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <steam/steam_api.h>

#include "GameAnalytics/GameAnalytics.h"
#include "StoreFront/GASteamFront.h"

constexpr const char* GAME_KEY   = "bd624ee6f8e6efb32a054f8d7ba11618";
constexpr const char* SECRET_KEY = "7f5c3f682cbd217841efba92e92ffb1b3b6612bc";

// one of Spacewar's achievements
constexpr const char* ACHIEVEMENT = "ACH_WIN_ONE_GAME";

int main()
{
    using namespace gameanalytics;
    using namespace std::chrono_literals;

    // 1. Steam first, exactly as the game would do it anyway
    if (!SteamAPI_Init())
    {
        std::cerr << "SteamAPI_Init failed: is the Steam client running and logged in, and is steam_appid.txt next to the executable?\n";
        return 1;
    }

    // 2. GameAnalytics, with the Steam storefront
    GameAnalytics::setEnabledInfoLog(true);
    GameAnalytics::setEnabledVerboseLog(true);

    GameAnalytics::configureBuild("steam-sample-1.0");
    GameAnalytics::configureAvailableResourceCurrencies({"gold"});
    GameAnalytics::configureAvailableResourceItemTypes({"loot", "shop"});

    GameAnalytics::configureStoreFront<GASteamFront>();

    GameAnalytics::initialize(GAME_KEY, SECRET_KEY);

    // re-lock the test achievement so the unlock below fires every run
    SteamUserStats()->ClearAchievement(ACHIEVEMENT);
    SteamUserStats()->StoreStats();

    // creating a lobby enters it, which the storefront reports as store:social:lobby_joined
    SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, 4);

    // 3. a handful of game events, sent once
    GameAnalytics::addProgressionEvent(EGAProgressionStatus::Complete, 100, "forest");
    GameAnalytics::addResourceEvent(EGAResourceFlowType::Source, "gold", 50.f, "loot", "chest");
    GameAnalytics::addDesignEvent("sample:jump", 3.);

    // let the re-lock above reach Steam first, otherwise lock + unlock in the same frame is no change and nothing is reported
    for (int i = 0; i < 20; ++i)
    {
        SteamAPI_RunCallbacks();
        std::this_thread::sleep_for(100ms);
    }

    // the game unlocks an achievement as usual; the storefront sees UserAchievementStored_t and sends the event
    const bool unlocked = SteamUserStats()->SetAchievement(ACHIEVEMENT) && SteamUserStats()->StoreStats();
    std::cout << "unlock " << ACHIEVEMENT << ": " << (unlocked ? "requested" : "rejected by Steam") << '\n';

    // 4. pump Steam callbacks for a few seconds so the storefront receives the achievement and lobby callbacks.
    //    No events are sent from this loop; a real game calls SteamAPI_RunCallbacks once per frame.
    for (int i = 0; i < 50; ++i)
    {
        SteamAPI_RunCallbacks();
        std::this_thread::sleep_for(100ms);
    }

    // 5. shut down: GameAnalytics first (stops the storefront), then Steam
    GameAnalytics::onQuit();
    SteamAPI_Shutdown();

    std::cout << "done\n";
    return 0;
}
