#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <GAHealth.h>
#include <nlohmann/json.hpp>

using namespace gameanalytics;
using ::testing::Return;

namespace gameanalytics
{
    class MockGAPlatform : public GAPlatform
	{
	public:
		MOCK_METHOD(std::string, getOSVersion, (), (override));
		MOCK_METHOD(std::string, getDeviceManufacturer, (), (override));
		MOCK_METHOD(std::string, getBuildPlatform, (), (override));
		MOCK_METHOD(std::string, getPersistentPath, (), (override));
		MOCK_METHOD(std::string, getDeviceModel, (), (override));
		MOCK_METHOD(std::string, getConnectionType, (), (override));
		
		// Mocking non-pure virtual methods
		MOCK_METHOD(std::string, getAdvertisingId, (), (override));
		MOCK_METHOD(std::string, getDeviceId, (), (override));
		MOCK_METHOD(void, setupUncaughtExceptionHandler, (), (override));
		MOCK_METHOD(void, onInit, (), (override));
		
		// Mocking const methods
		MOCK_METHOD(std::string, getCpuModel, (), (const, override));
		MOCK_METHOD(std::string, getGpuModel, (), (const, override));
		MOCK_METHOD(int, getNumCpuCores, (), (const, override));
		MOCK_METHOD(int64_t, getTotalDeviceMemory, (), (const, override));
		MOCK_METHOD(int64_t, getAppMemoryUsage, (), (const, override));
		MOCK_METHOD(int64_t, getSysMemoryUsage, (), (const, override));
		MOCK_METHOD(int64_t, getBootTime, (), (const, override));
		
		MockGAPlatform()
		{
			ON_CALL(*this, getOSVersion).WillByDefault(Return("10.0"));
			ON_CALL(*this, getDeviceManufacturer).WillByDefault(Return("GenericManufacturer"));
			ON_CALL(*this, getBuildPlatform).WillByDefault(Return("Windows"));
			ON_CALL(*this, getPersistentPath).WillByDefault(Return("/persistent/path"));
			ON_CALL(*this, getDeviceModel).WillByDefault(Return("DeviceModelX"));
			ON_CALL(*this, getConnectionType).WillByDefault(Return("WiFi"));
			
			ON_CALL(*this, getAdvertisingId).WillByDefault(Return("ad-id-123"));
			ON_CALL(*this, getDeviceId).WillByDefault(Return("device-id-456"));
			ON_CALL(*this, setupUncaughtExceptionHandler).WillByDefault(Return());
			ON_CALL(*this, onInit).WillByDefault(Return());
			
			ON_CALL(*this, getCpuModel).WillByDefault(Return("Intel Core i7"));
			ON_CALL(*this, getGpuModel).WillByDefault(Return("Nvidia GTX 1080"));
			ON_CALL(*this, getNumCpuCores).WillByDefault(Return(8));
			ON_CALL(*this, getTotalDeviceMemory).WillByDefault(Return(16384)); // 16GB
			ON_CALL(*this, getAppMemoryUsage).WillByDefault(Return(1024)); // 1GB
			ON_CALL(*this, getSysMemoryUsage).WillByDefault(Return(2048)); // 2GB
			ON_CALL(*this, getBootTime).WillByDefault(Return(30000)); // 30 seconds
		}
	};
}


// Test subclass to access protected members
class GAHealthTestable : public gameanalytics::GAHealth
{
public:
	using gameanalytics::GAHealth::GAHealth;  // Inherit constructor
	using gameanalytics::GAHealth::_fpsReadings;  // Expose protected member for testing
	using gameanalytics::GAHealth::_appMemoryUsage; // Expose protected memory usage for testing
	using gameanalytics::GAHealth::_sysMemoryUsage; // Expose system memory usage for testing
	using gameanalytics::GAHealth::getMemoryPercent;
	using gameanalytics::GAHealth::_totalMemory;
};


class GAHealthTest : public ::testing::Test
{
protected:
	MockGAPlatform* mockPlatform;
	GAHealthTestable* gaHealth;
	
	virtual void SetUp() override
	{
		mockPlatform = new MockGAPlatform();
		gaHealth = new GAHealthTestable(mockPlatform);
	}
	
	virtual void TearDown() override
	{
		delete gaHealth;
		delete mockPlatform;
	}
};

TEST_F(GAHealthTest, ConstructorInitializesPlatform)
{
	EXPECT_CALL(*mockPlatform, getCpuModel()).WillOnce(Return("Intel"));
	EXPECT_CALL(*mockPlatform, getNumCpuCores()).WillOnce(Return(4));
	EXPECT_CALL(*mockPlatform, getDeviceModel()).WillOnce(Return("Device123"));
	EXPECT_CALL(*mockPlatform, getGpuModel()).WillOnce(Return("Nvidia"));
	EXPECT_CALL(*mockPlatform, getTotalDeviceMemory()).WillOnce(Return(8192));
	
	gameanalytics::GAHealth health(mockPlatform);
	health.enableHardwareTracking = true;
	
	json out;
	health.addHealthAnnotations(out);
	
	std::cout << std::setw(4) << out << '\n';
	
	EXPECT_EQ(out["cpu_model"], "Intel");
	EXPECT_EQ(out["cpu_num_cores"], 4);
	EXPECT_EQ(out["hardware"], "Device123");
}

TEST_F(GAHealthTest, AddHealthAnnotationsIncludesHardwareTracking)
{
	EXPECT_CALL(*mockPlatform, getCpuModel()).WillOnce(Return("Intel"));
	EXPECT_CALL(*mockPlatform, getNumCpuCores()).WillOnce(Return(4));
	EXPECT_CALL(*mockPlatform, getDeviceModel()).WillOnce(Return("Device123"));
	
	GAHealthTestable* _localHealthTracker = new GAHealthTestable(mockPlatform);
	
	_localHealthTracker->enableHardwareTracking = true;
	
	json healthEvent;
	_localHealthTracker->addHealthAnnotations(healthEvent);
	
	std::cout << std::setw(4) << healthEvent["cpu_model"] << '\n';
	
	EXPECT_EQ(healthEvent["cpu_model"], "Intel");
	EXPECT_EQ(healthEvent["cpu_num_cores"], 4);
	EXPECT_EQ(healthEvent["hardware"], "Device123");
}

TEST_F(GAHealthTest, DoFpsReadingIncrementsBucketCorrectly)
{
	float testFps = 60.0f;
	
	gaHealth->doFpsReading(testFps);
	
	EXPECT_EQ(gaHealth->_fpsReadings[60], 1);
}

TEST_F(GAHealthTest, GetMemoryPercentReturnsCorrectValue)
{
	int percent = gaHealth->getMemoryPercent(4096);
	EXPECT_EQ(percent, 25); // 25% memory usage
}

// Test getMemoryPercent with various inputs
TEST_F(GAHealthTest, GetMemoryPercentReturnsCorrectValues)
{
    // Case 1: 50% memory usage
    int64_t totalMemory = 1000;
    gaHealth->_totalMemory = totalMemory;
    int memory = 500;
    int expectedPercent = 50;
    EXPECT_EQ(gaHealth->getMemoryPercent(memory), expectedPercent);

    // Case 2: 100% memory usage
    memory = 1000;
    expectedPercent = 100;
    EXPECT_EQ(gaHealth->getMemoryPercent(memory), expectedPercent);

    // Case 3: 0% memory usage
    memory = 1;
    expectedPercent = 0;
    EXPECT_EQ(gaHealth->getMemoryPercent(memory), expectedPercent);

    // Case 4: More than 100% memory usage (should not happen, but edge case)
    memory = 2000;
    expectedPercent = 100;  // Assuming 100% cap
    EXPECT_EQ(gaHealth->getMemoryPercent(memory), expectedPercent);

    // Case 5: Negative memory value (should return 0 or handle gracefully)
    memory = -100;
    expectedPercent = 0;  // Assuming a negative value will result in 0%
    EXPECT_EQ(gaHealth->getMemoryPercent(memory), expectedPercent);
}

TEST_F(GAHealthTest, AddPerformanceDataIncludesFPSTracking)
{
	gaHealth->enableFPSTracking = true;
	
	// Fill FPS readings with some values
	gaHealth->_fpsReadings[60] = 5;
	gaHealth->_fpsReadings[30] = 2;
	
	json performanceData;
	gaHealth->addPerformanceData(performanceData);
	
	
	json expectedFpsData;
	expectedFpsData["fps_data_table"] = gaHealth->_fpsReadings;
	
	EXPECT_EQ(performanceData["fps_data_table"], expectedFpsData["fps_data_table"]);
}

TEST_F(GAHealthTest, AddSDKInitDataIncludesBootTime)
{
	gaHealth->enableAppBootTimeTracking = true;
	
	EXPECT_CALL(*mockPlatform, getBootTime()).WillOnce(Return(5000));
	
	json sdkInitEvent;
	gaHealth->addSDKInitData(sdkInitEvent);
	
	EXPECT_EQ(sdkInitEvent["app_boot_time"], 5000);
}
