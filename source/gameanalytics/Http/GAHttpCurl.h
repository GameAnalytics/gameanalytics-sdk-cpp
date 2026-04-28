#pragma once

#include "GameAnalytics/GAHttpClient.h"

#ifdef GA_HTTP_CURL

#ifdef _WIN32

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include <windows.h>

#endif

#include <curl/curl.h>

namespace gameanalytics
{
    class GAHttpClientCurl: public GAHttpClient
    {
        virtual void initialize() override;
        
        virtual void cleanup() override;

        virtual Response sendRequest(
                std::string const& url,
                std::string const& auth, 
                std::vector<uint8_t> const& payloadData, 
                bool useGzip,
                void* userData) override;

        private:

        void createRequest(CURL *curl, std::string const& url, std::string const& auth, const std::vector<uint8_t>& payloadData, bool gzip);
    };
}

#endif // GA_HTTP_CURL
