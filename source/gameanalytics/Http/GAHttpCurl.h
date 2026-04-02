#pragma once

#include "GameAnalytics/GAHttpWrapper.h"
#include <curl/curl.h>

namespace gameanalytics
{
    class GAHttpCurl: public GAHttpWrapper
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