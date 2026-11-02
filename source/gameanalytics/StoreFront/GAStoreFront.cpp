#include "GameAnalytics/GAStoreFront.h"

#include "GameAnalytics/GameAnalytics.h"
#include "GACommon.h"
#include "GAState.h"
#include "GAThreading.h"
#include "GALogger.h"
#include "GAUtilities.h"

#ifdef GA_STORE_STEAM
    #include "StoreFront/GASteamFront.h"
#endif

#include <initializer_list>
#include <limits>

namespace gameanalytics
{
    namespace
    {
        // Reserved, store-agnostic design event namespace: "store:<kind>:<detail>". Every storefront produces the same ids,
        // so dashboards built for Steam keep working for Epic or GOG. Final names are still to be agreed with the backend (RESEARCH.md).
        constexpr const char* EVENT_PREFIX      = "store";
        constexpr const char* EVENT_ACHIEVEMENT = "achievement";
        constexpr const char* EVENT_PURCHASE    = "purchase";
        constexpr const char* EVENT_DLC         = "dlc";

        constexpr const char* CART_TYPE_STORE   = "store";

        // custom fields attached to every store event
        constexpr const char* FIELD_STORE       = "store";
        constexpr const char* FIELD_ORDER_ID    = "store_order_id";
        constexpr const char* FIELD_SERVER_TS   = "store_server_ts";
        constexpr const char* FIELD_QUANTITY    = "store_quantity";

        constexpr std::size_t MAX_EVENT_PART_LEN = 64;

        // Event parts allow A-Z a-z 0-9 space - _ . ( ) ! ? and at most 64 chars (GAValidator::validateEventPartCharacters).
        // Store ids (achievement API names, item names) are free-form, so anything else becomes '_' rather than failing validation.
        std::string sanitizeEventPart(std::string const& part)
        {
            std::string out;
            out.reserve(part.size());

            for (char c : part)
            {
                const unsigned char uc = static_cast<unsigned char>(c);
                const bool allowed = std::isalnum(uc) || c == ' ' || c == '-' || c == '_' || c == '.' || c == '(' || c == ')' || c == '!' || c == '?';
                out += allowed ? c : '_';
            }

            if (out.size() > MAX_EVENT_PART_LEN)
            {
                out.resize(MAX_EVENT_PART_LEN);
            }

            return out.empty() ? std::string(UNKNOWN_VALUE) : out;
        }

        std::string makeEventId(std::initializer_list<std::string> parts)
        {
            std::string id = EVENT_PREFIX;
            for (std::string const& part : parts)
            {
                id += ':';
                id += sanitizeEventPart(part);
            }
            return id;
        }

        std::string dumpFields(json const& fields)
        {
            try
            {
                return fields.empty() ? std::string() : fields.dump();
            }
            catch (std::exception const& e)
            {
                logging::GALogger::e("Storefront: failed to serialize custom fields: %s", e.what());
                return "";
            }
        }

    }

    // ---- generic store events ----

    namespace
    {
        std::string withStoreField(std::string const& storeName, std::string const& fieldsJson)
        {
            json fields = json::object();

            if (!fieldsJson.empty())
            {
                try
                {
                    fields = json::parse(fieldsJson);
                    if (!fields.is_object())
                    {
                        fields = json::object();
                    }
                }
                catch (std::exception const& e)
                {
                    logging::GALogger::w("Storefront: ignoring invalid custom fields: %s", e.what());
                    fields = json::object();
                }
            }

            fields[FIELD_STORE] = storeName;
            return dumpFields(fields);
        }
    }

    void GAStoreFront::sendStoreEvent(std::string const& kind, std::string const& detail, std::string const& fieldsJson)
    {
        GameAnalytics::addDesignEvent(makeEventId({kind, detail}), withStoreField(getName(), fieldsJson));
    }

    void GAStoreFront::sendStoreEvent(std::string const& kind, std::string const& detail, double value, std::string const& fieldsJson)
    {
        GameAnalytics::addDesignEvent(makeEventId({kind, detail}), value, withStoreField(getName(), fieldsJson));
    }

    // ---- event helpers ----
    //
    // These go through the public GameAnalytics API on purpose: it already hops to the GA thread, checks that the SDK is initialised
    // and enabled, parses the custom fields and runs the validators, so the storefront behaves exactly like a game sending the same event.
    // They are safe to call from any thread, including the store's callback thread.

    void GAStoreFront::sendAchievementEvent(std::string const& achievementId)
    {
        json fields;
        fields[FIELD_STORE] = getName();

        GameAnalytics::addDesignEvent(makeEventId({EVENT_ACHIEVEMENT, achievementId}), dumpFields(fields));
    }

    void GAStoreFront::sendPurchaseEvent(GAStorePurchase const& purchase)
    {
        json fields;
        fields[FIELD_STORE] = getName();

        if (!purchase.orderId.empty())
        {
            fields[FIELD_ORDER_ID] = purchase.orderId;
        }
        if (purchase.serverTimestamp > 0)
        {
            // business events have no timestamp parameter, so the store's server time only travels as a custom field
            fields[FIELD_SERVER_TS] = purchase.serverTimestamp;
        }

        if (purchase.authorized && purchase.hasPrice())
        {
            // GA business amounts are int cents
            if (purchase.amount > static_cast<int64_t>(std::numeric_limits<int>::max()))
            {
                logging::GALogger::w("Storefront: purchase amount %lld out of range, dropping business event (order %s)",
                                     static_cast<long long>(purchase.amount), purchase.orderId.c_str());
                return;
            }

            fields[FIELD_QUANTITY] = purchase.quantity;

            GameAnalytics::addBusinessEvent(purchase.currency,
                                            static_cast<int>(purchase.amount),
                                            sanitizeEventPart(purchase.itemType),
                                            sanitizeEventPart(purchase.itemId),
                                            CART_TYPE_STORE,
                                            dumpFields(fields));
            return;
        }

        // no price known on the client (Steam microtransactions): funnel event only, the order id lets the studio join it with their own data
        GameAnalytics::addDesignEvent(makeEventId({EVENT_PURCHASE, purchase.authorized ? "authorized" : "declined"}), dumpFields(fields));
    }

    void GAStoreFront::sendDlcEvent(std::string const& dlcId)
    {
        json fields;
        fields[FIELD_STORE] = getName();

        GameAnalytics::addDesignEvent(makeEventId({EVENT_DLC, dlcId}), dumpFields(fields));
    }

    void GAStoreFront::sendResourceEvent(EGAResourceFlowType flow, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId)
    {
        json fields;
        fields[FIELD_STORE] = getName();

        // currency and itemType must be in the configured lists (see publishResourceConfig); the validator logs if they are not
        GameAnalytics::addResourceEvent(flow, currency, amount, sanitizeEventPart(itemType), sanitizeEventPart(itemId), dumpFields(fields));
    }

    void GAStoreFront::publishResourceConfig(GAStoreResourceConfig const& config)
    {
        if (config.currencies.empty() && config.itemTypes.empty())
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([config]()
        {
            // This deliberately skips the "must be set before initialize()" rule of the public configure* API: the store's catalogue
            // arrives asynchronously after init, and a store-driven config only extends what the game set. GAState still validates
            // (max 20 entries, letters only for currencies, event-part characters for item types) and logs a warning when it rejects.
            //
            // LIMITATION: GAState only has replacing setters, so this overwrites any resource currencies / item types the game
            // configured itself. Merging with the game's lists needs GAState support that is not in place yet.
            if (!config.currencies.empty())
            {
                state::GAState::setAvailableResourceCurrencies(config.currencies);
            }

            if (!config.itemTypes.empty())
            {
                state::GAState::setAvailableResourceItemTypes(config.itemTypes);
            }

            logging::GALogger::i("Storefront: published %zu resource currencies and %zu item types from the store catalogue",
                                 config.currencies.size(), config.itemTypes.size());
        });
    }

    void GAStoreFront::requestQuit()
    {
        // This runs inside the store's callback dispatch (e.g. SteamShutdown_t), so it must neither block nor unregister callbacks
        // mid-dispatch. End the session on the GA thread and leave onQuit() (which waits for the thread and calls stop()) to the game.
        logging::GALogger::i("Storefront: %s is shutting down, ending session", getName().c_str());
        threading::GAThreading::performTaskOnGAThread([]()
        {
            state::GAState::endSessionAndStopQueue(false);
        });
    }

    // ---- factory ----

    std::unique_ptr<GAStoreFront> MakeStoreFront()
    {
#ifdef GA_STORE_STEAM
        // must run on the game's thread after the game's SteamAPI_Init(); if Steam is not up (yet), the SDK runs without a storefront
        if (GASteamFront::isSteamInitialized())
        {
            auto front = std::make_unique<GASteamFront>();
            if (front->isAvailable())
            {
                return front;
            }
            logging::GALogger::i("Steam is initialized but no valid user is logged in, running without a storefront");
        }
        else
        {
            logging::GALogger::d("Steam is not initialized, running without a storefront");
        }
#endif

        return nullptr;
    }
}
