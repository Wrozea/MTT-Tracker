#include <gtest/gtest.h>

#include <numbers>

#include "mtt/sim/motion.hpp"

using mtt::sim::KinematicState;
using mtt::sim::propagateConstantTurn;

namespace {

constexpr double kTolerance = 1e-9;

void expectStateNear(const KinematicState& actual, const KinematicState& expected) {
    EXPECT_NEAR(actual.position.x(), expected.position.x(), kTolerance);
    EXPECT_NEAR(actual.position.y(), expected.position.y(), kTolerance);
    EXPECT_NEAR(actual.velocity.x(), expected.velocity.x(), kTolerance);
    EXPECT_NEAR(actual.velocity.y(), expected.velocity.y(), kTolerance);
}

}  // namespace

TEST(ConstantTurn, ZeroTurnRateMovesInStraightLine) {
    const KinematicState start{{1.0, 2.0}, {3.0, -4.0}};
    const KinematicState end = propagateConstantTurn(start, 0.0, 2.0);
    expectStateNear(end, {{7.0, -6.0}, {3.0, -4.0}});
}

TEST(ConstantTurn, QuarterTurnLeftEndsOnCircle) {
    // Heading east at 10 m/s, turning left at pi/2 rad/s for one second: a
    // quarter circle of radius 10 / (pi/2), ending up heading north.
    const double turn_rate = std::numbers::pi / 2.0;
    const double radius = 10.0 / turn_rate;
    const KinematicState start{{0.0, 0.0}, {10.0, 0.0}};

    const KinematicState end = propagateConstantTurn(start, turn_rate, 1.0);

    expectStateNear(end, {{radius, radius}, {0.0, 10.0}});
}

TEST(ConstantTurn, NegativeTurnRateTurnsRight) {
    const double turn_rate = -std::numbers::pi / 2.0;
    const double radius = 10.0 / std::numbers::pi * 2.0;
    const KinematicState start{{0.0, 0.0}, {10.0, 0.0}};

    const KinematicState end = propagateConstantTurn(start, turn_rate, 1.0);

    expectStateNear(end, {{radius, -radius}, {0.0, -10.0}});
}

TEST(ConstantTurn, PreservesSpeed) {
    const KinematicState start{{5.0, 5.0}, {12.0, 5.0}};
    const KinematicState end = propagateConstantTurn(start, 0.7, 3.3);
    EXPECT_NEAR(end.velocity.norm(), 13.0, kTolerance);
}

TEST(ConstantTurn, ManyShortStepsMatchOneLongStep) {
    const KinematicState start{{-20.0, 40.0}, {8.0, 6.0}};
    const double turn_rate = 0.3;

    KinematicState stepped = start;
    for (int i = 0; i < 100; ++i) {
        stepped = propagateConstantTurn(stepped, turn_rate, 0.05);
    }
    const KinematicState direct = propagateConstantTurn(start, turn_rate, 5.0);

    expectStateNear(stepped, direct);
}
