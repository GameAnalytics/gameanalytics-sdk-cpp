#include "StoreFront/GASteamFront.h"

#ifdef GA_STORE_STEAM

#include "GALogger.h"
#include "GAUtilities.h"

#include <algorithm>
#include <cstdlib>

namespace gameanalytics
{
    namespace
    {
        constexpr const char* STEAM_HARDWARE_MANUFACTURER = "Valve";

        // custom item definition properties studios add in their Steamworks inventory schema for us (see RESEARCH.md)
        constexpr const char* PROP_NAME         = "name";
        constexpr const char* PROP_GA_CURRENCY  = "ga_currency";
        constexpr const char* PROP_GA_ITEM_TYPE = "ga_item_type";

        // per-instance properties as returned by the Web API item JSON; unverified that the client exposes them
        constexpr const char* PROP_ORIGIN       = "origin";
        constexpr const char* PROP_TRANS_ID     = "transid";
        constexpr const char* ORIGIN_PURCHASE   = "purchase";

        // inventory-derived resource flows have no real item type, they use this placeholder (added to the published item types)
        constexpr const char* RESOURCE_ITEM_TYPE_STORE = "store";
        constexpr const char* RESOURCE_ITEM_ID_INVENTORY = "inventory";

        // design event kind for social signals: "store:social:<detail>"
        constexpr const char* EVENT_SOCIAL = "social";

        std::string steamString(const char* s)
        {
            return s ? std::string(s) : std::string();
        }

        const char* formFactorName(ESteamDeviceFormFactor formFactor)
        {
            switch (formFactor)
            {
                case k_ESteamDeviceFormFactorPhone:     return "phone";
                case k_ESteamDeviceFormFactorTablet:    return "tablet";
                case k_ESteamDeviceFormFactorComputer:  return "computer";
                case k_ESteamDeviceFormFactorTV:        return "tv";
                default:                                return UNKNOWN_VALUE;   // includes newer values such as VR headsets
            }
        }
    }

    // ---- lifecycle ----

    bool GASteamFront::isSteamInitialized()
    {
        return SteamUser() != nullptr && SteamUtils() != nullptr && SteamApps() != nullptr;
    }

    GASteamFront::~GASteamFront()
    {
        stop();
    }

    bool GASteamFront::isAvailable() const
    {
        return isSteamInitialized() && SteamUser()->GetSteamID().IsValid();
    }

    void GASteamFront::start()
    {
        if (_started || !isAvailable())
        {
            return;
        }

        _started = true;

        cacheIdentity();
        cacheHardware();
        cacheAppContext();

        _cbAchievementStored.Register(this, &GASteamFront::onAchievementStored);
        _cbMicroTxnAuthorization.Register(this, &GASteamFront::onMicroTxnAuthorization);
        _cbDlcInstalled.Register(this, &GASteamFront::onDlcInstalled);
        _cbSteamShutdown.Register(this, &GASteamFront::onSteamShutdown);

        if (_socialEvents)
        {
            if (SteamFriends())
            {
                _cbPersonaStateChange.Register(this, &GASteamFront::onPersonaStateChange);
                _cbRichPresenceJoinRequested.Register(this, &GASteamFront::onRichPresenceJoinRequested);
                _cbLobbyJoinRequested.Register(this, &GASteamFront::onLobbyJoinRequested);
            }
            if (SteamMatchmaking())
            {
                _cbLobbyInvite.Register(this, &GASteamFront::onLobbyInvite);
                _cbLobbyEnter.Register(this, &GASteamFront::onLobbyEnter);
                _cbLobbyChatUpdate.Register(this, &GASteamFront::onLobbyChatUpdate);
                _cbLobbyGameCreated.Register(this, &GASteamFront::onLobbyGameCreated);
            }
            if (SteamRemotePlay())
            {
                _cbRemotePlayConnected.Register(this, &GASteamFront::onRemotePlayConnected);
                _cbRemotePlayDisconnected.Register(this, &GASteamFront::onRemotePlayDisconnected);
            }
        }

        if (SteamInventory())
        {
            _cbInventoryDefinitionUpdate.Register(this, &GASteamFront::onInventoryDefinitionUpdate);
            _cbInventoryResultReady.Register(this, &GASteamFront::onInventoryResultReady);

            // both are asynchronous; onInventoryDefinitionUpdate and onPricesReceived finish the catalogue setup
            SteamInventory()->LoadItemDefinitions();
            _priceRequest.Set(SteamInventory()->RequestPrices(), this, &GASteamFront::onPricesReceived);
        }

        logging::GALogger::i("Steam storefront started (app %u)", SteamUtils()->GetAppID());
    }

    void GASteamFront::stop()
    {
        if (!_started)
        {
            return;
        }

        _started = false;

        _cbAchievementStored.Unregister();
        _cbMicroTxnAuthorization.Unregister();
        _cbDlcInstalled.Unregister();
        _cbSteamShutdown.Unregister();
        _cbInventoryDefinitionUpdate.Unregister();
        _cbInventoryResultReady.Unregister();
        _cbPersonaStateChange.Unregister();
        _cbRichPresenceJoinRequested.Unregister();
        _cbLobbyJoinRequested.Unregister();
        _cbLobbyInvite.Unregister();
        _cbLobbyEnter.Unregister();
        _cbLobbyChatUpdate.Unregister();
        _cbLobbyGameCreated.Unregister();
        _cbRemotePlayConnected.Unregister();
        _cbRemotePlayDisconnected.Unregister();
        _priceRequest.Cancel();
        _friendsInGame.clear();

        if (SteamInventory())
        {
            std::lock_guard<std::mutex> lock(_inventoryMutex);
            for (SteamInventoryResult_t handle : _ownedResults)
            {
                SteamInventory()->DestroyResult(handle);
            }
            _ownedResults.clear();
        }
    }

    // ---- cached values ----

    void GASteamFront::cacheIdentity()
    {
        CSteamID id = SteamUser()->GetSteamID();

        // only real player accounts. GAState decides whether this becomes user_id or user_id_ext, and whether it is hashed
        if (id.IsValid() && id.BIndividualAccount())
        {
            _userId = std::to_string(id.ConvertToUint64());
        }
    }

    void GASteamFront::cacheHardware()
    {
        // requires Steamworks SDK 1.65+, which removed the older IsSteamRunningOnSteamDeck()
        switch (SteamUtils()->IsRunningOnSteamHardware())
        {
            case k_ESteamHardwareTypeSteamDeck:     _deviceModel = "Steam Deck";    break;
            case k_ESteamHardwareTypeSteamMachine:  _deviceModel = "Steam Machine"; break;
            case k_ESteamHardwareTypeSteamFrame:    _deviceModel = "Steam Frame";   break;
            default:                                                                break;
        }

        if (!_deviceModel.empty())
        {
            _deviceManufacturer = STEAM_HARDWARE_MANUFACTURER;
        }
    }

    void GASteamFront::cacheAppContext()
    {
        try
        {
            json fields;

            fields["steam_app_id"]   = SteamUtils()->GetAppID();
            fields["steam_build_id"] = SteamApps()->GetAppBuildId();
            fields["steam_language"] = steamString(SteamApps()->GetCurrentGameLanguage());
            fields["steam_country"]  = steamString(SteamUtils()->GetIPCountry());

            char beta[128] = {};
            if (SteamApps()->GetCurrentBetaName(beta, sizeof(beta)) && beta[0])
            {
                fields["steam_beta"] = beta;
            }

            uint32 secondsAllowed = 0;
            uint32 secondsPlayed  = 0;
            fields["steam_timed_trial"]    = SteamApps()->BIsTimedTrial(&secondsAllowed, &secondsPlayed);
            fields["steam_family_sharing"] = SteamApps()->BIsSubscribedFromFamilySharing();
            fields["steam_china"]          = SteamUtils()->IsSteamChinaLauncher();
            fields["steam_proton"]         = SteamUtils()->IsRunningUnderProton();

            _customFields = fields.dump();
        }
        catch (std::exception const& e)
        {
            logging::GALogger::e("Steam storefront: failed to build custom fields: %s", e.what());
        }
    }

    std::string GASteamFront::getUserId() const             { return _userId; }
    std::string GASteamFront::getDeviceModel() const        { return _deviceModel; }
    std::string GASteamFront::getDeviceManufacturer() const { return _deviceManufacturer; }
    std::string GASteamFront::getCustomFields() const       { return _customFields; }

    // ---- inventory: catalogue ----

    void GASteamFront::loadItemDefinitions()
    {
        uint32 count = 0;
        if (!SteamInventory()->GetItemDefinitionIDs(nullptr, &count) || count == 0)
        {
            return;
        }

        std::vector<SteamItemDef_t> ids(count);
        if (!SteamInventory()->GetItemDefinitionIDs(ids.data(), &count))
        {
            return;
        }
        ids.resize(count);

        auto property = [](SteamItemDef_t def, const char* name) -> std::string
        {
            char buffer[256] = {};
            uint32 size = sizeof(buffer) - 1;   // keep the last byte zero so the result is always terminated
            if (SteamInventory()->GetItemDefinitionProperty(def, name, buffer, &size))
            {
                return std::string(buffer);
            }
            return "";
        };

        std::lock_guard<std::mutex> lock(_inventoryMutex);
        for (SteamItemDef_t def : ids)
        {
            ItemDefinition& d = _itemDefinitions[def];   // keeps a price set by an earlier readPrices()
            d.name       = property(def, PROP_NAME);
            d.gaCurrency = property(def, PROP_GA_CURRENCY);
            d.gaItemType = property(def, PROP_GA_ITEM_TYPE);
        }

        logging::GALogger::d("Steam storefront: %u item definitions loaded", count);
    }

    void GASteamFront::readPrices()
    {
        uint32 count = SteamInventory()->GetNumItemsWithPrices();
        if (count == 0)
        {
            return;
        }

        std::vector<SteamItemDef_t> defs(count);
        std::vector<uint64> currentPrices(count);
        std::vector<uint64> basePrices(count);

        // The docs show GetItemsWithPrices(defs, prices, count); the SDK header has had a base-price array (pre-discount) since
        // discounts were added. Match whichever the SDK in STEAMWORKS_SDK_DIR declares.
        if (!SteamInventory()->GetItemsWithPrices(defs.data(), currentPrices.data(), basePrices.data(), count))
        {
            return;
        }

        std::lock_guard<std::mutex> lock(_inventoryMutex);
        for (uint32 i = 0; i < count; ++i)
        {
            _itemDefinitions[defs[i]].price = currentPrices[i];
        }
    }

    void GASteamFront::publishResourceConfigFromDefinitions()
    {
        GAStoreResourceConfig config;

        {
            std::lock_guard<std::mutex> lock(_inventoryMutex);
            for (auto const& [def, d] : _itemDefinitions)
            {
                if (!d.gaCurrency.empty() && !utilities::GAUtilities::stringVectorContainsString(config.currencies, d.gaCurrency))
                {
                    config.currencies.push_back(d.gaCurrency);
                }
                if (!d.gaItemType.empty() && !utilities::GAUtilities::stringVectorContainsString(config.itemTypes, d.gaItemType))
                {
                    config.itemTypes.push_back(d.gaItemType);
                }
            }
        }

        if (config.currencies.empty() && config.itemTypes.empty())
        {
            return;   // the studio did not tag anything for us, leave the game's own configuration alone
        }

        if (!utilities::GAUtilities::stringVectorContainsString(config.itemTypes, RESOURCE_ITEM_TYPE_STORE))
        {
            config.itemTypes.push_back(RESOURCE_ITEM_TYPE_STORE);
        }

        // GAValidator allows at most 20 of each; publishResourceConfig validates, logs and drops the excess
        publishResourceConfig(config);
    }

    // ---- inventory: results ----

    void GASteamFront::refreshInventory()
    {
        if (!_started || !SteamInventory())
        {
            return;
        }

        // Called from the GA thread on session start/end. Steam allocates thread-local memory for the calling thread, which only
        // SteamAPI_RunCallbacks frees, so release it here. The result itself arrives in onInventoryResultReady on the game's thread.
        // GetAllItems is rate limited and may return cached data if called too often, which is why this is tied to session boundaries.
        SteamInventoryResult_t handle = k_SteamInventoryResultInvalid;
        if (SteamInventory()->GetAllItems(&handle) && handle != k_SteamInventoryResultInvalid)
        {
            std::lock_guard<std::mutex> lock(_inventoryMutex);
            _ownedResults.push_back(handle);
        }

        SteamAPI_ReleaseCurrentThreadMemory();
    }

    bool GASteamFront::readItems(SteamInventoryResult_t handle, std::vector<SteamItemDetails_t>& outItems) const
    {
        // not OK for pending results, and (unverified) for a handle the game already destroyed
        if (SteamInventory()->GetResultStatus(handle) != k_EResultOK)
        {
            return false;
        }

        uint32 count = 0;
        if (!SteamInventory()->GetResultItems(handle, nullptr, &count))
        {
            return false;
        }

        outItems.resize(count);
        if (count > 0 && !SteamInventory()->GetResultItems(handle, outItems.data(), &count))
        {
            return false;
        }
        outItems.resize(count);

        return true;
    }

    std::string GASteamFront::readItemProperty(SteamInventoryResult_t handle, uint32_t index, const char* property) const
    {
        char buffer[256] = {};
        uint32 size = sizeof(buffer) - 1;
        if (SteamInventory()->GetResultItemProperty(handle, index, property, buffer, &size))
        {
            return std::string(buffer);
        }
        return "";
    }

    void GASteamFront::handleInventorySnapshot(SteamInventoryResult_t handle)
    {
        std::vector<SteamItemDetails_t> items;
        const bool ok = readItems(handle, items);
        SteamInventory()->DestroyResult(handle);

        if (!ok)
        {
            return;
        }

        std::map<SteamItemDef_t, uint32_t> current;
        for (SteamItemDetails_t const& item : items)
        {
            if (!(item.m_unFlags & k_ESteamItemRemoved))
            {
                current[item.m_iDefinition] += item.m_unQuantity;
            }
        }

        std::map<SteamItemDef_t, ItemDefinition> definitions;
        std::map<SteamItemDef_t, uint32_t>       previous;
        bool hadSnapshot = false;
        {
            std::lock_guard<std::mutex> lock(_inventoryMutex);
            definitions = _itemDefinitions;
            previous    = _inventorySnapshot;
            hadSnapshot = _hasSnapshot;

            _inventorySnapshot = current;
            _hasSnapshot       = true;
        }

        if (!hadSnapshot)
        {
            return;   // nothing to diff against yet. TODO load/store the snapshot through GAStore so this also works across runs
        }

        // Only definitions tagged as GA currencies become resource events: a quantity change of a plain item is not a currency flow in GA's
        // model, and purchases are handled in handleForeignResult. The reason for the change is unknown here, hence the placeholder type/id.
        std::map<SteamItemDef_t, int64_t> deltas;
        for (auto const& [def, qty] : current)  deltas[def] += static_cast<int64_t>(qty);
        for (auto const& [def, qty] : previous) deltas[def] -= static_cast<int64_t>(qty);

        for (auto const& [def, delta] : deltas)
        {
            if (delta == 0)
            {
                continue;
            }

            auto it = definitions.find(def);
            if (it == definitions.end() || it->second.gaCurrency.empty())
            {
                logging::GALogger::d("Steam storefront: inventory item %d changed by %lld, not a GA currency, ignored", def, static_cast<long long>(delta));
                continue;
            }

            sendResourceEvent(delta > 0 ? EGAResourceFlowType::Source : EGAResourceFlowType::Sink,
                              it->second.gaCurrency,
                              static_cast<float>(std::llabs(delta)),
                              RESOURCE_ITEM_TYPE_STORE,
                              RESOURCE_ITEM_ID_INVENTORY);
        }
    }

    void GASteamFront::handleForeignResult(SteamInventoryResult_t handle)
    {
        // A result the game asked for: StartPurchase, ConsumeItem, ExchangeItems or its own GetAllItems. We must not destroy it, and the
        // game's handler may already have destroyed it if it ran before ours (listener order is undocumented), in which case readItems fails.
        std::vector<SteamItemDetails_t> items;
        if (!readItems(handle, items) || items.empty())
        {
            return;
        }

        std::map<SteamItemDef_t, ItemDefinition> definitions;
        std::string currency;
        {
            std::lock_guard<std::mutex> lock(_inventoryMutex);
            definitions = _itemDefinitions;
            currency    = _currency;
        }

        if (currency.empty())
        {
            return;   // prices not known yet, nothing we can price
        }

        const int64_t serverTimestamp = SteamInventory()->GetResultTimestamp(handle);

        for (uint32 i = 0; i < items.size(); ++i)
        {
            SteamItemDetails_t const& item = items[i];

            if (item.m_unFlags & (k_ESteamItemRemoved | k_ESteamItemConsumed))
            {
                continue;
            }

            // Without the per-instance origin we cannot tell a purchase result from the game's own full inventory listing,
            // so we only report items that say they were purchased rather than invent sales.
            if (readItemProperty(handle, i, PROP_ORIGIN) != ORIGIN_PURCHASE)
            {
                continue;
            }

            auto def = definitions.find(item.m_iDefinition);
            if (def == definitions.end() || def->second.price == 0)
            {
                continue;
            }

            GAStorePurchase purchase;
            purchase.orderId         = readItemProperty(handle, i, PROP_TRANS_ID);
            purchase.currency        = currency;
            purchase.amount          = static_cast<int64_t>(def->second.price) * item.m_unQuantity;   // minor units; see RESEARCH.md on zero-decimal currencies and price drift
            purchase.itemType        = def->second.gaItemType.empty() ? RESOURCE_ITEM_TYPE_STORE : def->second.gaItemType;
            purchase.itemId          = def->second.name.empty() ? std::to_string(item.m_iDefinition) : def->second.name;
            purchase.quantity        = item.m_unQuantity;
            purchase.serverTimestamp = serverTimestamp;

            sendPurchaseEvent(purchase);
        }
    }

    // ---- Steam callbacks (game's thread) ----

    void GASteamFront::onAchievementStored(UserAchievementStored_t* p)
    {
        if (!p)
        {
            return;
        }

        logging::GALogger::d("Steam storefront: UserAchievementStored_t %s (%u/%u)", p->m_rgchAchievementName, p->m_nCurProgress, p->m_nMaxProgress);

        // both zero means fully unlocked; anything else is progress from IndicateAchievementProgress, which is too noisy to send
        if (p->m_nCurProgress == 0 && p->m_nMaxProgress == 0)
        {
            sendAchievementEvent(p->m_rgchAchievementName);
        }
    }

    void GASteamFront::onMicroTxnAuthorization(MicroTxnAuthorizationResponse_t* p)
    {
        if (!p || p->m_unAppID != SteamUtils()->GetAppID())
        {
            return;
        }

        // the client never sees amount, currency or items for this flow (they live on the studio's purchasing server),
        // so this ends up as a design event carrying the order id
        GAStorePurchase purchase;
        purchase.orderId    = std::to_string(p->m_ulOrderID);
        purchase.authorized = p->m_bAuthorized != 0;

        sendPurchaseEvent(purchase);
    }

    void GASteamFront::onDlcInstalled(DlcInstalled_t* p)
    {
        if (p)
        {
            sendDlcEvent(std::to_string(p->m_nAppID));
        }
    }

    void GASteamFront::onSteamShutdown(SteamShutdown_t*)
    {
        requestQuit();
    }

    void GASteamFront::onInventoryDefinitionUpdate(SteamInventoryDefinitionUpdate_t*)
    {
        loadItemDefinitions();
        readPrices();   // prices may already have arrived
        publishResourceConfigFromDefinitions();
    }

    void GASteamFront::onPricesReceived(SteamInventoryRequestPricesResult_t* result, bool ioFailure)
    {
        if (ioFailure || !result || result->m_result != k_EResultOK)
        {
            logging::GALogger::w("Steam storefront: RequestPrices failed (%d)", result ? static_cast<int>(result->m_result) : -1);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(_inventoryMutex);
            _currency = result->m_rgchCurrency;
        }

        readPrices();
    }

    void GASteamFront::onInventoryResultReady(SteamInventoryResultReady_t* p)
    {
        if (!p || !SteamInventory())
        {
            return;
        }

        bool ours = false;
        {
            std::lock_guard<std::mutex> lock(_inventoryMutex);
            auto it = std::find(_ownedResults.begin(), _ownedResults.end(), p->m_handle);
            if (it != _ownedResults.end())
            {
                ours = true;
                _ownedResults.erase(it);
            }
        }

        if (ours)
        {
            if (p->m_result == k_EResultOK)
            {
                handleInventorySnapshot(p->m_handle);   // destroys the handle
            }
            else
            {
                SteamInventory()->DestroyResult(p->m_handle);
            }
        }
        else if (p->m_result == k_EResultOK)
        {
            handleForeignResult(p->m_handle);
        }
    }

    // ---- social callbacks (game's thread) ----
    //
    // Counts and flags only: other players' SteamIDs, persona names and chat content never leave the process.
    // Everything goes out as "store:social:<detail>" design events through GAStoreFront::sendStoreEvent.

    void GASteamFront::onPersonaStateChange(PersonaStateChange_t* p)
    {
        // fires for every friend and every status change; we only care about friends entering or leaving this game
        if (!p || !(p->m_nChangeFlags & (k_EPersonaChangeGamePlayed | k_EPersonaChangeGoneOffline)))
        {
            return;
        }

        const CSteamID friendId(p->m_ulSteamID);
        if (friendId == SteamUser()->GetSteamID() || !SteamFriends()->HasFriend(friendId, k_EFriendFlagImmediate))
        {
            return;
        }

        FriendGameInfo_t game = {};
        const bool inThisGame = SteamFriends()->GetFriendGamePlayed(friendId, &game) && game.m_gameID.AppID() == SteamUtils()->GetAppID();
        const bool wasInThisGame = _friendsInGame.count(p->m_ulSteamID) > 0;

        if (inThisGame == wasInThisGame)
        {
            return;   // only transitions, otherwise a friend changing status repeatedly would spam events
        }

        if (inThisGame)
        {
            _friendsInGame.insert(p->m_ulSteamID);
        }
        else
        {
            _friendsInGame.erase(p->m_ulSteamID);
        }

        // value = number of friends currently playing this game
        sendStoreEvent(EVENT_SOCIAL,inThisGame ? "friend_started_playing" : "friend_stopped_playing", static_cast<double>(_friendsInGame.size()));
    }

    void GASteamFront::onRichPresenceJoinRequested(GameRichPresenceJoinRequested_t*)
    {
        // the user joined a friend's game from the friends list or accepted an invite; the connect string stays with the game
        sendStoreEvent(EVENT_SOCIAL,"joined_via_friend");
    }

    void GASteamFront::onLobbyJoinRequested(GameLobbyJoinRequested_t* p)
    {
        // m_steamIDFriend is invalid when the join did not come through a friend
        const bool viaFriend = p && p->m_steamIDFriend.IsValid();
        sendStoreEvent(EVENT_SOCIAL,viaFriend ? "lobby_join_via_friend" : "lobby_join_requested");
    }

    void GASteamFront::onLobbyInvite(LobbyInvite_t* p)
    {
        if (!p || CGameID(p->m_ulGameID).AppID() != SteamUtils()->GetAppID())
        {
            return;
        }

        sendStoreEvent(EVENT_SOCIAL,"invite_received");
    }

    void GASteamFront::onLobbyEnter(LobbyEnter_t* p)
    {
        if (!p)
        {
            return;
        }

        if (p->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess)
        {
            json fields;
            fields["lobby_locked"]  = p->m_bLocked;
            fields["lobby_members"] = SteamMatchmaking()->GetNumLobbyMembers(CSteamID(p->m_ulSteamIDLobby));

            sendStoreEvent(EVENT_SOCIAL,"lobby_joined", fields.dump());
        }
        else
        {
            // value = EChatRoomEnterResponse (full, banned, limited account, ...)
            sendStoreEvent(EVENT_SOCIAL,"lobby_join_failed", static_cast<double>(p->m_EChatRoomEnterResponse));
        }
    }

    void GASteamFront::onLobbyChatUpdate(LobbyChatUpdate_t* p)
    {
        // our own join/leave is covered by LobbyEnter
        if (!p || p->m_ulSteamIDUserChanged == SteamUser()->GetSteamID().ConvertToUint64())
        {
            return;
        }

        const char* detail = nullptr;
        if (p->m_rgfChatMemberStateChange & k_EChatMemberStateChangeEntered)
        {
            detail = "lobby_member_joined";
        }
        else if (p->m_rgfChatMemberStateChange & (k_EChatMemberStateChangeLeft | k_EChatMemberStateChangeDisconnected | k_EChatMemberStateChangeKicked | k_EChatMemberStateChangeBanned))
        {
            detail = "lobby_member_left";
        }

        if (!detail)
        {
            return;
        }

        // value = lobby size after the change
        sendStoreEvent(EVENT_SOCIAL,detail, static_cast<double>(SteamMatchmaking()->GetNumLobbyMembers(CSteamID(p->m_ulSteamIDLobby))));
    }

    void GASteamFront::onLobbyGameCreated(LobbyGameCreated_t* p)
    {
        if (!p)
        {
            return;
        }

        // a game server was set for the lobby, i.e. the match starts; value = players in the lobby
        sendStoreEvent(EVENT_SOCIAL,"match_started", static_cast<double>(SteamMatchmaking()->GetNumLobbyMembers(CSteamID(p->m_ulSteamIDLobby))));
    }

    void GASteamFront::onRemotePlayConnected(SteamRemotePlaySessionConnected_t* p)
    {
        if (!p)
        {
            return;
        }

        json fields;
        fields["remote_play_device"]   = formFactorName(SteamRemotePlay()->GetSessionClientFormFactor(p->m_unSessionID));
        fields["remote_play_sessions"] = SteamRemotePlay()->GetSessionCount();

        sendStoreEvent(EVENT_SOCIAL,"remote_play_started", fields.dump());
    }

    void GASteamFront::onRemotePlayDisconnected(SteamRemotePlaySessionDisconnected_t*)
    {
        // value = sessions still connected
        sendStoreEvent(EVENT_SOCIAL,"remote_play_ended", static_cast<double>(SteamRemotePlay()->GetSessionCount()));
    }
}

#endif // GA_STORE_STEAM
