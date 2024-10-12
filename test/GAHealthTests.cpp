#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <GAHealth.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace gameanalytics;


class MockGAPlatform : public GAPlatform
{
public:
	// Mock the pure virtual methods
	MOCK_METHOD(std::string, getOSVersion, (), (override));
	MOCK_METHOD(std::string, getDeviceManufacturer, (), (override));
	MOCK_METHOD(std::string, getBuildPlatform, (), (override));
	MOCK_METHOD(std::string, getPersistentPath, (), (override));
	MOCK_METHOD(std::string, getDeviceModel, (), (override));
	MOCK_METHOD(std::string, getConnectionType, (), (override));
	
	MOCK_METHOD(std::string, getAdvertisingId, (), (override));
	MOCK_METHOD(std::string, getDeviceId, (), (override));
	MOCK_METHOD(void, setupUncaughtExceptionHandler, (), (override));
	MOCK_METHOD(void, onInit, (), (override));
	
	MOCK_METHOD(std::string, getCpuModel, (), (const, override));
	MOCK_METHOD(std::string, getGpuModel, (), (const, override));
	MOCK_METHOD(int, getNumCpuCores, (), (const, override));
	MOCK_METHOD(int64_t, getTotalDeviceMemory, (), (const, override));
	MOCK_METHOD(int64_t, getAppMemoryUsage, (), (const, override));
	MOCK_METHOD(int64_t, getSysMemoryUsage, (), (const, override));
	MOCK_METHOD(int64_t, getBootTime, (), (const, override));
};

// Test subclass to access protected members
class TestableGAHealth : public GAHealth
{
public:
	using GAHealth::GAHealth;
	
	// Expose protected members for testing
	using gameanalytics::GAHealth::_cpuModel;
	using gameanalytics::GAHealth::_numCores;
	using gameanalytics::GAHealth::_hardware;
	using gameanalytics::GAHealth::_gpuModel;
	using gameanalytics::GAHealth::_fpsReadings;
	using gameanalytics::GAHealth::_appMemoryUsage;
	using gameanalytics::GAHealth::_sysMemoryUsage;
	using gameanalytics::GAHealth::_totalMemory;
	
	// Expose protected methods for testing
	using gameanalytics::GAHealth::getMemoryPercent;
};


class GAHealthTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		platform = new MockGAPlatform();
		EXPECT_CALL(*platform, getCpuModel()).WillOnce(::testing::Return("Mock CPU"));
		EXPECT_CALL(*platform, getNumCpuCores()).WillOnce(::testing::Return(8));
		EXPECT_CALL(*platform, getDeviceModel()).WillOnce(::testing::Return("Mock Device"));
		EXPECT_CALL(*platform, getGpuModel()).WillOnce(::testing::Return("Mock GPU"));
		EXPECT_CALL(*platform, getTotalDeviceMemory()).WillOnce(::testing::Return(16000));
		
		health = new TestableGAHealth(platform);
	}
	
	void TearDown() override
	{
		delete health;
		delete platform;
	}
	
	TestableGAHealth* health;
	MockGAPlatform* platform;
};

// Test 1: Constructor Initialization
TEST_F(GAHealthTest, ConstructorInitializesPlatformValues)
{
	EXPECT_EQ(health->_cpuModel, "Mock CPU");
	EXPECT_EQ(health->_numCores, 8);
	EXPECT_EQ(health->_hardware, "Mock Device");
	EXPECT_EQ(health->_gpuModel, "Mock GPU");
	EXPECT_EQ(health->_totalMemory, 16000);
}

// Test 2: doFpsReading Method
TEST_F(GAHealthTest, DoFpsReadingAddsToBucket)
{
	health->doFpsReading(30.0f);
	health->doFpsReading(30.4f);
	
	EXPECT_EQ(health->_fpsReadings[30], 2);
}

// Test 3: doAppMemoryReading Method
TEST_F(GAHealthTest, DoAppMemoryReadingIncrementsAppMemoryUsage)
{
	health->doAppMemoryReading(4000);
	int memoryPercent = health->getMemoryPercent(4000);
	
	EXPECT_EQ(health->_appMemoryUsage[memoryPercent], 1);
}

// Test 4: addHealthAnnotations Method
TEST_F(GAHealthTest, AddHealthAnnotations)
{
	json healthEvent;
	
	health->enableHardwareTracking = true;
	health->enableMemoryTracking = true;
	
	health->addHealthAnnotations(healthEvent);
	
	EXPECT_EQ(healthEvent["cpu_model"], "Mock CPU");
	EXPECT_EQ(healthEvent["hardware"], "Mock Device");
	EXPECT_EQ(healthEvent["cpu_num_cores"], 8);
	EXPECT_EQ(healthEvent["memory_sys_total"], 16000);
}

// Test 5: getMemoryPercent Method
TEST_F(GAHealthTest, GetMemoryPercentCorrectValue)
{
	EXPECT_EQ(health->getMemoryPercent(8000), 50); // Half of 16000
	EXPECT_EQ(health->getMemoryPercent(16000), 100); // Full memory
	
	EXPECT_EQ(health->getMemoryPercent(1), 0);  // Very small usage
}

// Test group for edge cases
TEST_F(GAHealthTest, GetMemoryPercentEdgeCases) {
	// Test when memory is exactly 0
	EXPECT_EQ(health->getMemoryPercent(0), -1);
	
	// Test negative memory values
	EXPECT_EQ(health->getMemoryPercent(-1), -1);
	EXPECT_EQ(health->getMemoryPercent(-500), -1);
	
}

// Test 6: addPerformanceData Method
TEST_F(GAHealthTest, AddPerformanceData)
{
	health->enableFPSTracking = true;
	health->enableMemoryTracking = true;
	
	health->doFpsReading(30);
	health->doAppMemoryReading(4000);
	health->doSysMemoryReading(8000);
	
	json healthEvent;
	health->addPerformanceData(healthEvent);
	
	EXPECT_EQ(healthEvent["fps_data_table"][30], 1);
	EXPECT_EQ(healthEvent["memory_sys_data_table"][50], 1);  // 8000 out of 16000
	EXPECT_EQ(healthEvent["memory_app_data_table"][25], 1);  // 4000 out of 16000
}

// Test 7: addSDKInitData Method
TEST_F(GAHealthTest, AddSDKInitData)
{
	EXPECT_CALL(*platform, getBootTime()).WillOnce(::testing::Return(5000));
	health->enableAppBootTimeTracking = true;
	
	json sdkInitEvent;
	health->addSDKInitData(sdkInitEvent);
	
	EXPECT_EQ(sdkInitEvent["app_boot_time"], 5000);
}


