#include "gtest/gtest.h"
#include "MockLidar.h"

TEST(LidarTest, InitializeSuccess) {
    MockLidar mockLidar;
    EXPECT_CALL(mockLidar, initialize())
        .WillOnce(::testing::Return(true));

    EXPECT_TRUE(mockLidar.initialize());
}

TEST(LidarTest, TurnOnSuccess) {
    MockLidar mockLidar;
    EXPECT_CALL(mockLidar, initialize()).WillOnce(::testing::Return(true));
    EXPECT_CALL(mockLidar, turnOn()).WillOnce(::testing::Return(true));

    EXPECT_TRUE(mockLidar.initialize());
    EXPECT_TRUE(mockLidar.turnOn());
}

TEST(LidarTest, ProcessDataSuccess) {
    MockLidar mockLidar;
    LaserScan scan;

    EXPECT_CALL(mockLidar, doProcessSimple(::testing::_))
        .WillOnce(::testing::Return(true));

    EXPECT_TRUE(mockLidar.doProcessSimple(scan));
}
