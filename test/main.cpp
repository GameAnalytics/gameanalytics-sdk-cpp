#include <gtest/gtest.h>

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	
	if (sizeof(void*) == 8) {
        std::cout << "64-bit architecture" << std::endl;
    } else if (sizeof(void*) == 4) {
        std::cout << "32-bit architecture" << std::endl;
    } else {
        std::cout << "Unknown architecture" << std::endl;
    }
	
	// Run only a specific test, e.g., "GATests.testInitialize"
//	::testing::GTEST_FLAG(filter) = "GATests.testInitialize";
	
	return RUN_ALL_TESTS();
}
