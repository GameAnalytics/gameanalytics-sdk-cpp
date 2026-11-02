#pragma once

#include "GameAnalytics/GATypes.h"

#include <string>
#include <cstdint>
#include <memory>

namespace gameanalytics
{
    // A storefront integration (Steam, Epic, GOG, ...). See RESEARCH.md ("Recommended design: an internal GAStoreFront").
    //
    // The game picks one with GameAnalytics::configureStoreFront<T>() after initialising the store's own SDK and before
    // GameAnalytics::initialize(). From then on the SDK drives it: it takes the user id, device info and custom fields from it,
    // and the storefront hooks into the store's own callbacks to send events by itself. The game never calls it directly.
    //
    // Threading: the SDK calls start()/stop() on the thread that called configureStoreFront()/onQuit(), i.e. the game's thread.
    // Getters must return values cached in start(), because the SDK reads them from its own thread, where calling into a store SDK
    // is usually not safe (Steam allocates thread-local memory that only SteamAPI_RunCallbacks frees). Store callbacks arrive on
    // whatever thread the game pumps them from; the protected send*/publish* helpers hand the work to the SDK thread.

    // A purchase seen through the store. What is filled in depends on the flow:
    //  - Steam microtransactions (ISteamMicroTxn): only orderId and authorized. The amount lives on the studio's purchasing server,
    //    so this can only become a design event.
    //  - Steam Inventory purchases (ISteamInventory): currency and amount come from RequestPrices/GetItemPrice, items from the
    //    SteamInventoryResultReady_t result, so this becomes a real business event.
    struct GAStorePurchase
    {
        std::string orderId;            // store transaction / order id, for joining with the studio's own data
        bool        authorized = true;  // false = user declined (microtransactions only)

        // business event fields, leave empty/0 when the store does not expose them
        std::string currency;           // ISO 4217, e.g. "USD"
        int64_t     amount   = 0;       // total price in the currency's minor unit, i.e. cents (price * quantity)
        std::string itemType;           // GA item type / category, from the store's item catalogue
        std::string itemId;             // store item definition id or name
        uint32_t    quantity = 1;
        int64_t     serverTimestamp = 0;// unix time from the store if available, 0 = now

        bool hasPrice() const { return !currency.empty() && amount > 0; }
    };

    // Resource configuration derived from the store's item catalogue (e.g. Steam Inventory item definitions tagged with
    // "ga_currency" / "ga_item_type" custom properties). Arrives asynchronously after initialize(); it extends what the game configured.
    struct GAStoreResourceConfig
    {
        StringVector currencies;        // GA rules: max 20, letters only
        StringVector itemTypes;         // GA rules: max 20, max 32 chars
    };

    class GAStoreFront
    {
        public:

            virtual ~GAStoreFront() = default;

            // the store's client is running and the game has initialised its SDK
            // (e.g. SteamAPI_Init succeeded). false = the SDK ignores the storefront.
            virtual bool isAvailable() const = 0;

            // hook into / unhook from the store's callbacks and cache the values below
            virtual void start() = 0;
            virtual void stop()  = 0;

            virtual std::string getName()   const = 0;      // "steam"
            virtual std::string getUserId() const = 0;      // e.g. SteamID64, raw. Applied like configureUserId(); a later configureUserId() replaces it

            // only when the store knows better than the OS, otherwise empty
            virtual std::string getDeviceModel()        const { return ""; }   // "Steam Deck"
            virtual std::string getDeviceManufacturer() const { return ""; }   // "Valve"

            // store context merged into the global custom event fields, as a JSON object string.
            // e.g. app id, build id, beta branch, language, country. The game's own global fields win on conflicts.
            virtual std::string getCustomFields() const { return ""; }

            // Called by the SDK on every session start, for stores whose inventory has no push notification (Steam: GetAllItems diff).
            // Default does nothing. Implementations compare against the snapshot they persisted and call sendResourceEvent for deltas.
            virtual void refreshInventory() {}

        protected:

            // Shared by all stores so events look the same whatever the store. Safe to call from any thread.
            // Event ids are design events under a reserved, store-agnostic prefix: "store:<kind>:<detail>".

            void sendAchievementEvent(std::string const& achievementId);           // "store:achievement:<id>"

            // business event when purchase.hasPrice(), otherwise design event "store:purchase:authorized|declined" with the order id
            void sendPurchaseEvent(GAStorePurchase const& purchase);

            void sendDlcEvent(std::string const& dlcId);                           // "store:dlc:<id>"

            // for inventory-derived resource flows; itemType/itemId may be placeholders when the store gives no reason for the change
            void sendResourceEvent(EGAResourceFlowType flow, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId);

            // generic "store:<kind>:<detail>" design event for anything without a dedicated helper above (social, context).
            // fieldsJson is a JSON object string or empty; the "store" field is added automatically.
            void sendStoreEvent(std::string const& kind, std::string const& detail, std::string const& fieldsJson = "");
            void sendStoreEvent(std::string const& kind, std::string const& detail, double value, std::string const& fieldsJson = "");

            // adds catalogue-derived resource currencies/item types to the configured lists (allowed after initialize(), unlike the public API)
            void publishResourceConfig(GAStoreResourceConfig const& config);

            // the store is shutting down (Steam: SteamShutdown_t): ends the session. The game is still expected to call onQuit().
            void requestQuit();
    };

    // Used by GameAnalytics::initialize() when the game did not call configureStoreFront() itself: returns the storefront for a
    // store that was compiled in (GA_STORE_STEAM) and is running, otherwise nullptr. Called on the game's thread.
    std::unique_ptr<GAStoreFront> MakeStoreFront();
}
