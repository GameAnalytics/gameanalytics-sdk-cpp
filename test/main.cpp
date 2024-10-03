#include <gtest/gtest.h>

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	
	// Run only a specific test, e.g., "GATests.testInitialize"
//	::testing::GTEST_FLAG(filter) = "GATests.testInitialize";
	
	return RUN_ALL_TESTS();
}
