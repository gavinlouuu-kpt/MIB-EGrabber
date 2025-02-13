#include <gtest/gtest.h>
#include "image_processing/image_processing.h"

class ImageProcessingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup code that will be called before each test
    }

    void TearDown() override
    {
        // Cleanup code that will be called after each test
    }
};

// A simple dummy test for addition
TEST_F(ImageProcessingTest, SimpleAddition)
{
    int result = 2 + 2;
    EXPECT_EQ(result, 4);
}

// A dummy test showing different assertions
TEST_F(ImageProcessingTest, DifferentAssertions)
{
    std::string test_string = "Hello";

    // Equal assertion
    EXPECT_EQ(test_string.length(), 5);

    // True/False assertions
    EXPECT_TRUE(test_string.length() > 0);
    EXPECT_FALSE(test_string.empty());

    // String comparison
    EXPECT_STREQ(test_string.c_str(), "Hello");
}