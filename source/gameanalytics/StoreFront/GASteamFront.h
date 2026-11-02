#pragma once

#ifdef GA_STORE_STEAM

#include "GameAnalytics/GAStoreFront.h"
#include "GACommon.h"

// the game's own Steamworks SDK, pointed to by STEAMWORKS_SDK_DIR; we never ship these headers (see RESEARCH.md, licensing)
#include <steam/steam_api.h>

#include <map>
#include <set>

namespace gameanalytics
{
    // DRAFT, see RESEARCH.md. Steam implementation of GAStoreFront.
    //
    // Lifecycle: the game calls SteamAPI_Init() and pumps SteamAPI_RunCallbacks(); we never do either.
    // MakeStoreFront() constructs this on the game's thread only when isSteamInitialized() is true, and the SDK
    // calls start()/stop() there. Steam callbacks arrive on whatever thread the game pumps from; every handler
    // hands its work to the GA thread through the GAStoreFront send*/publish* helpers and touches no SDK state itself.
    class GASteamFront final : public GAStoreFront
    {
        public:

            GASteamFront() = default;
            ~GASteamFront() override;

            // the interface accessors are null until SteamAPI_Init() succeeded (unverified against the docs, test it)
            static bool isSteamInitialized();

            bool isAvailable() const override;

            void start() override;
            void stop()  override;

            std::string getName()   const override { return "steam"; }
            std::string getUserId() const override;

            std::string getDeviceModel()        const override;
            std::string getDeviceManufacturer() const override;
            std::string getCustomFields()       const override;

            // issues GetAllItems(); the result arrives in onInventoryResultReady and is diffed against the previous snapshot
            void refreshInventory() override;

        private:

            struct ItemDefinition
            {
                std::string name;           // "name" property (English)
                std::string gaCurrency;     // "ga_currency" custom property: this definition is a GA resource currency with that name
                std::string gaItemType;     // "ga_item_type" custom property: GA item type for resource and business events
                uint64_t    price = 0;      // current price in the user's currency, minor units; 0 = not purchasable
            };

            // cached in start() on the game's thread, read from the GA thread through the getters
            std::string _userId;
            std::string _deviceModel;
            std::string _deviceManufacturer;
            std::string _customFields;
            bool        _started = false;

            // social events are high volume for some games; TODO: per-category opt-in switch (RESEARCH.md, decision 5)
            bool        _socialEvents = true;

            // friends currently playing this game, by SteamID64; touched on the callback thread only, never sent
            std::set<uint64_t> _friendsInGame;

            // inventory state, touched from the game's callback thread and from refreshInventory() on the GA thread
            mutable std::mutex                          _inventoryMutex;
            std::string                                 _currency;              // ISO 4217, from SteamInventoryRequestPricesResult_t
            std::map<SteamItemDef_t, ItemDefinition>    _itemDefinitions;
            std::map<SteamItemDef_t, uint32_t>          _inventorySnapshot;     // definition -> total quantity. TODO persist via GAStore so diffs survive restarts
            bool                                        _hasSnapshot = false;
            std::vector<SteamInventoryResult_t>         _ownedResults;          // handles from our own GetAllItems calls; we destroy these, never the game's

            void cacheIdentity();
            void cacheHardware();
            void cacheAppContext();

            void loadItemDefinitions();
            void readPrices();
            void publishResourceConfigFromDefinitions();

            void handleInventorySnapshot(SteamInventoryResult_t handle);    // our GetAllItems result -> resource events from the diff
            void handleForeignResult(SteamInventoryResult_t handle);        // the game's result (StartPurchase etc.) -> business events

            bool        readItems(SteamInventoryResult_t handle, std::vector<SteamItemDetails_t>& outItems) const;
            std::string readItemProperty(SteamInventoryResult_t handle, uint32_t index, const char* property) const;

            // manual callbacks so registration happens in start(), after Steam is known to be initialised, and not at construction
            STEAM_CALLBACK_MANUAL(GASteamFront, onAchievementStored,         UserAchievementStored_t,          _cbAchievementStored);
            STEAM_CALLBACK_MANUAL(GASteamFront, onMicroTxnAuthorization,     MicroTxnAuthorizationResponse_t,  _cbMicroTxnAuthorization);
            STEAM_CALLBACK_MANUAL(GASteamFront, onDlcInstalled,              DlcInstalled_t,                   _cbDlcInstalled);
            STEAM_CALLBACK_MANUAL(GASteamFront, onSteamShutdown,             SteamShutdown_t,                  _cbSteamShutdown);
            STEAM_CALLBACK_MANUAL(GASteamFront, onInventoryDefinitionUpdate, SteamInventoryDefinitionUpdate_t, _cbInventoryDefinitionUpdate);
            STEAM_CALLBACK_MANUAL(GASteamFront, onInventoryResultReady,      SteamInventoryResultReady_t,      _cbInventoryResultReady);

            // social: ISteamFriends / ISteamMatchmaking / ISteamRemotePlay. Counts and flags only, never other players' ids or names.
            STEAM_CALLBACK_MANUAL(GASteamFront, onPersonaStateChange,        PersonaStateChange_t,                 _cbPersonaStateChange);
            STEAM_CALLBACK_MANUAL(GASteamFront, onRichPresenceJoinRequested, GameRichPresenceJoinRequested_t,      _cbRichPresenceJoinRequested);
            STEAM_CALLBACK_MANUAL(GASteamFront, onLobbyJoinRequested,        GameLobbyJoinRequested_t,             _cbLobbyJoinRequested);
            STEAM_CALLBACK_MANUAL(GASteamFront, onLobbyInvite,               LobbyInvite_t,                        _cbLobbyInvite);
            STEAM_CALLBACK_MANUAL(GASteamFront, onLobbyEnter,                LobbyEnter_t,                         _cbLobbyEnter);
            STEAM_CALLBACK_MANUAL(GASteamFront, onLobbyChatUpdate,           LobbyChatUpdate_t,                    _cbLobbyChatUpdate);
            STEAM_CALLBACK_MANUAL(GASteamFront, onLobbyGameCreated,          LobbyGameCreated_t,                   _cbLobbyGameCreated);
            STEAM_CALLBACK_MANUAL(GASteamFront, onRemotePlayConnected,       SteamRemotePlaySessionConnected_t,    _cbRemotePlayConnected);
            STEAM_CALLBACK_MANUAL(GASteamFront, onRemotePlayDisconnected,    SteamRemotePlaySessionDisconnected_t, _cbRemotePlayDisconnected);

            // RequestPrices() is a call result (only the caller gets it), which is fine because we are the caller
            CCallResult<GASteamFront, SteamInventoryRequestPricesResult_t> _priceRequest;
            void onPricesReceived(SteamInventoryRequestPricesResult_t* result, bool ioFailure);
    };
}

#endif // GA_STORE_STEAM
