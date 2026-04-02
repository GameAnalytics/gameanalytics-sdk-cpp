//
// GA-SDK-CPP
// Tests for the HTTP interface abstraction and custom implementation registration
//

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "GameAnalytics/GameAnalytics.h"
#include "GameAnalytics/GAHttpWrapper.h"
#include "GAHTTPApi.h"

namespace
{

// A mock HTTP implementation for testing
class MockHttpClient : public gameanalytics::GAHttpWrapper
{
public:
    void initialize() override
    {
        initialized = true;
    }

    void cleanup() override
    {
        cleanedUp = true;
    }

    Response sendRequest(
        std::string const& url,
        std::string const& auth,
        std::vector<uint8_t> const& payloadData,
        bool useGzip,
        void* userData) override
    {
        lastUrl = url;
        lastAuth = auth;
        lastPayload = payloadData;
        lastUseGzip = useGzip;
        requestCount++;

        return configuredResponse;
    }

    // Test inspection
    bool initialized = false;
    bool cleanedUp = false;
    int requestCount = 0;
    std::string lastUrl;
    std::string lastAuth;
    std::vector<uint8_t> lastPayload;
    bool lastUseGzip = false;

    // Configurable response
    Response configuredResponse = {};
};

// -------- Response struct tests --------

TEST(GAHttpWrapperResponse, DefaultResponseHasNegativeCode)
{
    gameanalytics::GAHttpWrapper::Response response;
    EXPECT_EQ(response.code, -1);
    EXPECT_TRUE(response.packet.empty());
}

TEST(GAHttpWrapperResponse, ToStringReturnsEmptyForEmptyPacket)
{
    gameanalytics::GAHttpWrapper::Response response;
    EXPECT_TRUE(response.toString().empty());
}

TEST(GAHttpWrapperResponse, ToStringReturnsPacketContent)
{
    gameanalytics::GAHttpWrapper::Response response;
    std::string body = R"({"status":"ok"})";
    response.packet.assign(body.begin(), body.end());
    response.code = 200;

    std::string_view result = response.toString();
    EXPECT_EQ(result, body);
    EXPECT_EQ(result.size(), body.size());
}

TEST(GAHttpWrapperResponse, ToStringHandlesBinaryData)
{
    gameanalytics::GAHttpWrapper::Response response;
    response.packet = {0x00, 0x01, 0x02, 0xFF};
    response.code = 200;

    std::string_view result = response.toString();
    EXPECT_EQ(result.size(), 4u);
}

// -------- Mock implementation tests --------

TEST(GAHttpInterface, MockImplementsInterface)
{
    auto mock = std::make_unique<MockHttpClient>();
    EXPECT_FALSE(mock->initialized);
    EXPECT_FALSE(mock->cleanedUp);

    mock->initialize();
    EXPECT_TRUE(mock->initialized);

    mock->cleanup();
    EXPECT_TRUE(mock->cleanedUp);
}

TEST(GAHttpInterface, MockSendRequestRecordsParameters)
{
    MockHttpClient mock;
    mock.configuredResponse.code = 200;
    std::string responseBody = R"({"ok":true})";
    mock.configuredResponse.packet.assign(responseBody.begin(), responseBody.end());

    std::string url = "https://api.gameanalytics.com/v2/test/events";
    std::string auth = "Authorization: abc123";
    std::vector<uint8_t> payload = {'[', '{', '}', ']'};

    auto response = mock.sendRequest(url, auth, payload, true, nullptr);

    EXPECT_EQ(mock.requestCount, 1);
    EXPECT_EQ(mock.lastUrl, url);
    EXPECT_EQ(mock.lastAuth, auth);
    EXPECT_EQ(mock.lastPayload, payload);
    EXPECT_TRUE(mock.lastUseGzip);
    EXPECT_EQ(response.code, 200);
    EXPECT_EQ(response.toString(), responseBody);
}

TEST(GAHttpInterface, MockCanReturnErrorResponse)
{
    MockHttpClient mock;
    mock.configuredResponse.code = 500;
    std::string body = "Internal Server Error";
    mock.configuredResponse.packet.assign(body.begin(), body.end());

    auto response = mock.sendRequest("http://test.com", "auth", {}, false, nullptr);

    EXPECT_EQ(response.code, 500);
    EXPECT_EQ(response.toString(), body);
}

TEST(GAHttpInterface, MockCanReturnNoContentResponse)
{
    MockHttpClient mock;
    mock.configuredResponse.code = 204;
    // 204 has no body

    auto response = mock.sendRequest("http://test.com", "auth", {}, false, nullptr);

    EXPECT_EQ(response.code, 204);
    EXPECT_TRUE(response.packet.empty());
}

// -------- Registration tests --------

TEST(GAHttpInterface, SetCustomHttpImplAcceptsUniquePtr)
{
    // Verify the static method compiles and runs without crashing
    auto mock = std::make_unique<MockHttpClient>();
    gameanalytics::http::GAHTTPApi::setCustomHttpImpl(std::move(mock));

    // Clean up: reset to nullptr so it doesn't affect other tests
    gameanalytics::http::GAHTTPApi::setCustomHttpImpl(nullptr);
}

} // namespace
