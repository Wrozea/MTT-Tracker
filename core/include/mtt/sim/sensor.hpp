#pragma once

#include <vector>

#include <Eigen/Core>

#include "mtt/random.hpp"
#include "mtt/sim/scenario.hpp"
#include "mtt/types.hpp"

namespace mtt::sim {

struct Area {
    double min_x;
    double max_x;
    double min_y;
    double max_y;

    bool contains(const Eigen::Vector2d& point) const;
    double size() const;
};

struct SensorConfig {
    Area coverage{-500.0, 500.0, -500.0, 500.0};
    double detection_probability = 0.9;
    double position_noise_stddev = 5.0;  // meters, applied to x and y separately
    double clutter_rate = 5.0;           // expected false alarms per frame
};

inline constexpr int kClutterOrigin = -1;

struct SensorReturn {
    std::vector<Detection> detections;
    // origins[i] is the truth id that caused detections[i], or kClutterOrigin.
    // Simulation-only label for the dashboard and for tests. The tracker is
    // given detections alone.
    std::vector<int> origins;
};

// Turns true target positions into what a radar-like sensor would report:
// noisy positions, occasional misses, and random false alarms, in a random
// order so that list position reveals nothing about origin.
class Sensor {
public:
    Sensor(SensorConfig config, Rng rng);

    SensorReturn observe(const std::vector<TruthTarget>& targets);

    const SensorConfig& config() const { return config_; }

private:
    void addTargetDetections(const std::vector<TruthTarget>& targets, SensorReturn& out);
    void addClutter(SensorReturn& out);
    void shuffleReturn(SensorReturn& out);

    SensorConfig config_;
    Rng rng_;
};

}  // namespace mtt::sim
