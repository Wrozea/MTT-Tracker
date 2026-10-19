#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <vector>

#include "mtt/sim/scenario.hpp"
#include "mtt/sim/simulator.hpp"

using mtt::sim::makeDemoScenario;
using mtt::sim::Scenario;
using mtt::sim::SimFrame;
using mtt::sim::Simulator;
using mtt::sim::SimulatorConfig;
using mtt::sim::TargetSpec;
using mtt::sim::TruthTarget;

namespace {

std::vector<SimFrame> runFrames(Simulator& simulator, int count) {
    std::vector<SimFrame> frames;
    for (int i = 0; i < count; ++i) {
        frames.push_back(simulator.step());
    }
    return frames;
}

std::optional<TruthTarget> findTruth(const SimFrame& frame, int id) {
    const auto it = std::find_if(frame.truth.begin(), frame.truth.end(),
                                 [id](const TruthTarget& t) { return t.id == id; });
    if (it == frame.truth.end()) {
        return std::nullopt;
    }
    return *it;
}

Scenario singleTarget(TargetSpec spec) {
    Scenario scenario;
    scenario.targets.push_back(std::move(spec));
    return scenario;
}

}  // namespace

TEST(Simulator, SameSeedReproducesEveryFrameExactly) {
    SimulatorConfig config;
    config.seed = 99;
    Simulator a(makeDemoScenario(), config);
    Simulator b(makeDemoScenario(), config);

    for (int i = 0; i < 400; ++i) {
        const SimFrame frame_a = a.step();
        const SimFrame frame_b = b.step();
        ASSERT_EQ(frame_a.sensor.origins, frame_b.sensor.origins);
        for (std::size_t j = 0; j < frame_a.sensor.detections.size(); ++j) {
            ASSERT_EQ(frame_a.sensor.detections[j].position, frame_b.sensor.detections[j].position);
        }
    }
}

TEST(Simulator, DifferentSeedsGiveDifferentDetections) {
    SimulatorConfig config_a;
    config_a.seed = 1;
    SimulatorConfig config_b;
    config_b.seed = 2;
    Simulator a(makeDemoScenario(), config_a);
    Simulator b(makeDemoScenario(), config_b);

    const SimFrame frame_a = a.step();
    const SimFrame frame_b = b.step();
    EXPECT_NE(frame_a.sensor.detections.front().position, frame_b.sensor.detections.front().position);
}

TEST(Simulator, FrameTimeIsIndexTimesPeriod) {
    Simulator simulator(makeDemoScenario(), SimulatorConfig{});
    const std::vector<SimFrame> frames = runFrames(simulator, 41);
    EXPECT_EQ(frames[0].time, 0.0);
    EXPECT_DOUBLE_EQ(frames[40].time, 2.0);
    EXPECT_EQ(frames[40].index, 40);
}

TEST(Simulator, TargetAppearsAtSpawnAndDisappearsAtDespawn) {
    Simulator simulator(singleTarget({.id = 5,
                                      .spawn_time = 1.0,
                                      .despawn_time = 2.0,
                                      .initial = {{0.0, 0.0}, {1.0, 0.0}},
                                      .turns = {}}),
                        SimulatorConfig{});
    const std::vector<SimFrame> frames = runFrames(simulator, 50);

    EXPECT_FALSE(findTruth(frames[19], 5).has_value());  // t = 0.95 s
    EXPECT_TRUE(findTruth(frames[20], 5).has_value());   // t = 1.00 s
    EXPECT_TRUE(findTruth(frames[39], 5).has_value());   // t = 1.95 s
    EXPECT_FALSE(findTruth(frames[40], 5).has_value());  // t = 2.00 s
}

TEST(Simulator, TargetStartsFromInitialStateWhenSpawned) {
    Simulator simulator(singleTarget({.id = 1,
                                      .spawn_time = 1.0,
                                      .despawn_time = 10.0,
                                      .initial = {{10.0, -20.0}, {4.0, 0.0}},
                                      .turns = {}}),
                        SimulatorConfig{});
    const std::vector<SimFrame> frames = runFrames(simulator, 41);

    EXPECT_EQ(findTruth(frames[20], 1)->state.position, Eigen::Vector2d(10.0, -20.0));
    // One second later at 4 m/s: 4 m east.
    EXPECT_NEAR(findTruth(frames[40], 1)->state.position.x(), 14.0, 1e-9);
}

TEST(Simulator, TargetIsRetiredWhenItLeavesCoverage) {
    // Starts 10 m inside the east edge, moving east at 100 m/s: out in 0.1 s.
    Simulator simulator(singleTarget({.id = 1,
                                      .spawn_time = 0.0,
                                      .despawn_time = 100.0,
                                      .initial = {{490.0, 0.0}, {100.0, 0.0}},
                                      .turns = {}}),
                        SimulatorConfig{});
    const std::vector<SimFrame> frames = runFrames(simulator, 10);

    EXPECT_TRUE(findTruth(frames[2], 1).has_value());   // x = 500, on the edge
    EXPECT_FALSE(findTruth(frames[3], 1).has_value());  // x = 505
    // A retired target never comes back.
    for (std::size_t i = 3; i < frames.size(); ++i) {
        EXPECT_FALSE(findTruth(frames[i], 1).has_value());
    }
}

TEST(Simulator, TurnCommandTakesEffectAtItsStartTime) {
    Simulator simulator(singleTarget({.id = 1,
                                      .spawn_time = 0.0,
                                      .despawn_time = 100.0,
                                      .initial = {{0.0, 0.0}, {10.0, 0.0}},
                                      .turns = {{1.0, 0.5}}}),
                        SimulatorConfig{});
    const std::vector<SimFrame> frames = runFrames(simulator, 22);

    EXPECT_NEAR(findTruth(frames[20], 1)->state.velocity.y(), 0.0, 1e-12);  // straight until t = 1
    EXPECT_GT(findTruth(frames[21], 1)->state.velocity.y(), 0.0);           // turning left after
}

TEST(DemoScenario, TargetsTwoAndThreeMeetAtTenSeconds) {
    Simulator simulator(makeDemoScenario(), SimulatorConfig{});
    const std::vector<SimFrame> frames = runFrames(simulator, 201);

    const TruthTarget two = *findTruth(frames[200], 2);
    const TruthTarget three = *findTruth(frames[200], 3);
    EXPECT_NEAR((two.state.position - three.state.position).norm(), 0.0, 1e-9);
    EXPECT_NEAR(two.state.position.x(), -200.0, 1e-9);
    EXPECT_NEAR(two.state.position.y(), 50.0, 1e-9);
}

TEST(DemoScenario, TargetFiveCompletesHalfTurn) {
    Simulator simulator(makeDemoScenario(), SimulatorConfig{});
    const std::vector<SimFrame> frames = runFrames(simulator, 521);

    const TruthTarget five = *findTruth(frames[520], 5);  // t = 26 s
    EXPECT_NEAR(five.state.velocity.x(), 0.0, 1e-9);
    EXPECT_NEAR(five.state.velocity.y(), -20.0, 1e-9);
}
