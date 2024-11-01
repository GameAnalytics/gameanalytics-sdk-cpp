//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include "GACommon.h"
#include <vector>
#include <string>
#include <locale>
#include <codecvt>
#include <exception>

namespace gameanalytics
{
    namespace utilities
    {
        template<typename ...args_t>
        std::string printString(std::string const& fmt, args_t&&... args)
        {
            constexpr int k_maxLogSize = 2048;
            char buffer[k_maxLogSize] = "";

            std::snprintf(buffer, k_maxLogSize, fmt.c_str(), std::forward<args_t>(args)...);

            return buffer;
        }

        template<typename T>
        T getOptionalValue(json& node, std::string const& key, T const& defaultValue = {})
        {
            try
            {
                if(node.contains(key))
                    return node[key].get<T>();
            }
            catch(json::exception const& e)
            {
                // in case of error just return the default value
                (void)e;
            }

            return defaultValue;
        }

        inline double convertBytesToMB(int64_t bytes)
        {
            constexpr double TO_MB = 1024 * 1024;
            return std::round(static_cast<double>(bytes) / TO_MB);
        }

        static inline bool addIfNotEmpty(json& out, std::string const& key, std::string const& str)
        {
            if (!key.empty() && !str.empty())
            {
                out[key] = str;
                return true;
            }

            return false;
        }

        static inline bool copyValueIfExistent(json& out, const json& in, std::string const& key)
        {
            if(in.contains(key))
            {
                out[key] = in[key];
                return true;
            }

            return false;
        }

        inline std::string trimString(std::string const& str, std::size_t size)
        {
            return str.substr(0, std::min(size, str.size()));
        }

        inline json parseFields(std::string const& fields)
        {
            try
            {
                return fields.empty() ? json() : json::parse(fields);
            }
            catch(const std::exception& e)
            {
                (void)e;
                return json();
            }
        }

        inline std::int64_t getTimestamp()
        {
            return std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count();
        }

        inline std::string toLowerCase(std::string const& str)
        {
            std::string s = str;
            std::transform(s.begin(), s.end(), s.begin(), 
                [](char a) -> char {return std::tolower(a); });

            return s;
        }

        inline std::string toUpperCase(std::string const& str)
        {
            std::string s = str;
            std::transform(s.begin(), s.end(), s.begin(), 
                [](char a) -> char {return std::toupper(a); });

            return s;
        }

        std::string printArray(const StringVector& v, std::string const& delimiter = ", ");
        int64_t getNumberFromCache(json& node, std::string const& key, int64_t defValue = 0ll);

        std::pair<std::string, int32_t> getRelevantFunctionFromCallStack();

        struct GAUtilities
        {
            static std::string generateUUID();
            static void hmacWithKey(const char* key, const std::vector<uint8_t>& data, std::vector<uint8_t>& out);
            static bool stringMatch(std::string const& string, std::string const& pattern);
            static std::vector<uint8_t> gzipCompress(const char* data);

            // added for C++ port
            static bool isStringNullOrEmpty(const char* s);
            static bool stringVectorContainsString(const StringVector& vector, const std::string& search);
            static int64_t timeIntervalSince1970();
            static int base64_needed_encoded_length(int length_of_data);
            static void base64_encode(const unsigned char * src, int src_len, unsigned char *buf_);

            inline static std::string ws2s(const std::wstring& wstr)
            {
                try
                {
                    return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wstr);
                }
                catch(const std::exception& e)
                {
                    return "";
                }
            }

            inline static std::wstring s2ws(const std::string& str)
            {
                try
                {
                    return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(str);
                }
                catch(const std::exception& e)
                {
                    return L"";
                }
            }
        };
    }
}
