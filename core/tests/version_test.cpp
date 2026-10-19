#include <gtest/gtest.h>

#include "mtt/version.hpp"

TEST(Version, MatchesProjectVersion) {
    EXPECT_EQ(mtt::version(), "0.1.0");
}
