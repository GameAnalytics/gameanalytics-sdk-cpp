#include "GAHttpStub.h"
#include "GALogger.h"

namespace gameanalytics
{
    void GAHttpClientStub::printWarning() const
    {
        logging::GALogger::w("No proper HTTP client has been registered." 
            "Compile with \'GA_HTTP_USE_CURL\' to use the default GameAnalytics client" 
            "or provide a custom implementation using \'GameAnalytics::configureHttpClient\'");
    }

    void GAHttpClientStub::initialize()
    {
        logging::GALogger::d("GameAnalytics HTTP Client stub - initialize");
        printWarning();
    }

    void GAHttpClientStub::cleanup()
    {
        logging::GALogger::d("GameAnalytics HTTP Client stub - cleanup");
        printWarning();
    }

    GAHttpClient::Response GAHttpClientStub::sendRequest(std::string const& url, std::string const& auth, std::vector<uint8_t> const& payloadData, bool useGzip, void* userData)
    {
        logging::GALogger::d("GameAnalytics HTTP Client stub - send request: url: %s, content: %.*s", url.c_str(), (int)payloadData.size(), payloadData.data());
        printWarning();
    }
}