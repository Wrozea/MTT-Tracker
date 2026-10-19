#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "mtt/random.hpp"
#include "mtt/sim/sensor.hpp"

using mtt::Rng;
using mtt::sim::kClutterOrigin;
using mtt::sim::Sensor;
using mtt::sim::SensorConfig;
using mtt::sim::SensorReturn;
using mtt::sim::TruthTarget;

namespace {

SensorConfig perfectSensor() {
    SensorConfig config;
    config.detection_probability = 1.0;
    config.position_noise_stddev = 0.0;
    config.clutter_rate = 0.0;
    return config;
}

std::vector<TruthTarget> twoTargets() {
    return {{7, {{10.0, 20.0}, {1.0, 0.0}}}, {9, {{-30.0, 40.0}, {0.0, 1.0}}}};
}

}  // namespace

TEST(Sensor, PerfectSensorReportsExactTruePositions) {
    Sensor sensor(perfectSensor(), Rng(1));
    const std::vector<TruthTarget> targets = twoTargets();
    const SensorReturn result = sensor.observe(targets);

    ASSERT_EQ(result.detections.size(), 2u);
    for (std::size_t i = 0; i < result.detections.size(); ++i) {
        const TruthTarget& target = result.origins[i] == 7 ? targets[0] : targets[1];
        EXPECT_EQ(result.detections[i].position, target.state.position);
    }
}

TEST(Sensor, ZeroDetectionProbabilityReportsNothing) {
    SensorConfig config = perfectSensor();
    config.detection_probability = 0.0;
    Sensor sensor(config, Rng(2));

    EXPECT_TRUE(sensor.observe(twoTargets()).detections.empty());
}

TEST(Sensor, TargetOutsideCoverageIsNeverDetected) {
    Sensor sensor(perfectSensor(), Rng(3));
    const std::vector<TruthTarget> outside{{1, {{900.0, 0.0}, {0.0, 0.0}}}};

    EXPECT_TRUE(sensor.observe(outside).detections.empty());
}

TEST(Sensor, DetectionRateMatchesProbability) {
    SensorConfig config = perfectSensor();
    config.detection_probability = 0.8;
    Sensor sensor(config, Rng(4));

    int detected = 0;
    constexpr int kFrames = 20000;
    for (int i = 0; i < kFrames; ++i) {
        detected += static_cast<int>(sensor.observe({twoTargets()[0]}).detections.size());
    }
    EXPECT_NEAR(static_cast<double>(detected) / kFrames, 0.8, 0.01);
}

TEST(Sensor, NoiseHasConfiguredStandardDeviation) {
    SensorConfig config = perfectSensor();
    config.position_noise_stddev = 5.0;
    Sensor sensor(config, Rng(5));
    const TruthTarget target = twoTargets()[0];

    double squared_error_x = 0.0;
    double squared_error_y = 0.0;
    constexpr int kFrames = 20000;
    for (int i = 0; i < kFrames; ++i) {
        // Spelled out rather than auto: auto would hold Eigen's unevaluated
        // expression, which keeps a reference into the temporary SensorReturn.
        const Eigen::Vector2d error = sensor.observe({target}).detections[0].position - target.state.position;
        squared_error_x += error.x() * error.x();
        squared_error_y += error.y() * error.y();
    }
    EXPECT_NEAR(std::sqrt(squared_error_x / kFrames), 5.0, 0.1);
    EXPECT_NEAR(std::sqrt(squared_error_y / kFrames), 5.0, 0.1);
}

TEST(Sensor, ClutterCountAveragesRateAndStaysInCoverage) {
    SensorConfig config = perfectSensor();
    config.clutter_rate = 6.0;
    Sensor sensor(config, Rng(6));

    int total = 0;
    constexpr int kFrames = 5000;
    for (int i = 0; i < kFrames; ++i) {
        const SensorReturn result = sensor.observe({});
        total += static_cast<int>(result.detections.size());
        for (std::size_t j = 0; j < result.detections.size(); ++j) {
            ASSERT_TRUE(config.coverage.contains(result.detections[j].position));
            ASSERT_EQ(result.origins[j], kClutterOrigin);
        }
    }
    EXPECT_NEAR(static_cast<double>(total) / kFrames, 6.0, 0.15);
}

TEST(Sensor, OutputOrderDoesNotRevealOrigin) {
    SensorConfig config = perfectSensor();
    config.clutter_rate = 3.0;
    Sensor sensor(config, Rng(7));

    int target_detection_first = 0;
    int frames_with_both = 0;
    for (int i = 0; i < 5000; ++i) {
        const SensorReturn result = sensor.observe({twoTargets()[0]});
        if (result.origins.size() < 2) {
            continue;
        }
        ++frames_with_both;
        target_detection_first += result.origins[0] == 7 ? 1 : 0;
    }
    // Without shuffling the target detection would always come first.
    EXPECT_LT(static_cast<double>(target_detection_first) / frames_with_both, 0.5);
}
