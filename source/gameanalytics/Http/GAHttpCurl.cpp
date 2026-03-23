#include "Http/GAHttpCurl.h"
#include "GAHTTPApi.h"
#include "GALogger.h"

namespace gameanalytics
{
    size_t writefunc(void *ptr, size_t size, size_t nmemb, GAHttpWrapper::Response *s)
    {
        if(!s || !ptr)
        {
            return 0;
        }

        const size_t new_len = s->packet.size() + size * nmemb + 1;
        s->packet.reserve(new_len);
        s->packet.insert(s->packet.end(), reinterpret_cast<char*>(ptr), reinterpret_cast<char*>(ptr) + size * nmemb);

        return size*nmemb;
    }

    void GAHttpCurl::initialize()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    void GAHttpCurl::cleanup()
    {
        curl_global_cleanup();
    }

    GAHttpWrapper::Response GAHttpCurl::sendRequest(std::string const& url, std::string const& auth, std::vector<uint8_t> const& payloadData, bool useGzip, void* userData)
    {
        CURL* curl = curl_easy_init();
        if (!curl)
        {
            return {};
        }

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        GAHttpWrapper::Response response = {};

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        createRequest(curl, url, auth, payloadData, useGzip);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            logging::GALogger::d("%s", curl_easy_strerror(res));
            return {};
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.code);
        curl_easy_cleanup(curl);

        return response;
    }

    void GAHttpCurl::createRequest(CURL *curl, std::string const& url, std::string const& auth, const std::vector<uint8_t>& payloadData, bool gzip)
    {
        if(!curl)
        {
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        struct curl_slist *header = NULL;

        if (gzip)
        {
            header = curl_slist_append(header, "Content-Encoding: gzip");
        }

        header = curl_slist_append(header, auth.c_str());

        // always JSON
        header = curl_slist_append(header, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadData.data());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payloadData.size());
    }
}