//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include <vector>
#include "GACommon.h"
#include "GameAnalytics.h"
#include <string>
#include <locale>
#include <codecvt>
#include <exception>
#include "GALogger.h"

namespace gameanalytics
{
    namespace utilities
    {
        static inline void addIfNotEmpty(json& out, std::string const& key, std::string const& str)
        {
            if (!key.empty() && !str.empty())
                out[key] = str;
        }

        static inline int64_t getTimestamp()
        {
            return std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count();
        }

        static inline std::string toLowerCase(std::string const& str)
        {
            std::string s = str;
            std::transform(s.begin(), s.end(), s.begin(), 
                [](char a) -> char {return std::tolower(a); });

            return s;
        }


        class GAUtilities
        {
        public:
            static const char* getPathSeparator();
            static std::string generateUUID();
            static void hmacWithKey(const char* key, const std::vector<char>& data, char* out);
            static bool stringMatch(std::string const& string, std::string const& pattern);
            static std::vector<char> gzipCompress(const char* data);

            // added for C++ port
            static bool isStringNullOrEmpty(const char* s);
            static void uppercaseString(char* s);
            static void lowercaseString(char* s);
            static bool stringVectorContainsString(const StringVector& vector, const std::string& search);
            static int64_t timeIntervalSince1970();
            static void printJoinStringArray(const StringVector& v, const char* format, const char* delimiter = ", ");

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
                    logging::GALogger::d("Error with ws2s: %S", wstr.c_str());
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
                    logging::GALogger::d("Error with s2ws: %s", str.c_str());
                    return L"";
                }
            }

        private:
            static char pathSeparator[];
        };
    }
}
