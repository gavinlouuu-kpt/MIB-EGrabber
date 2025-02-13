#include <gtest/gtest.h>
#include "mib_grabber/mib_grabber.h"

class MibGrabberTest : public ::testing::Test
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

TEST_F(MibGrabberTest, BasicTest)
{
    // Add your first test here
    EXPECT_TRUE(true); // Placeholder test
}