#include "mtt/sim/sensor.hpp"

#include <numeric>
#include <utility>

namespace mtt::sim {

bool Area::contains(const Eigen::Vector2d& point) const {
    return point.x() >= min_x && point.x() <= max_x && point.y() >= min_y && point.y() <= max_y;
}

double Area::size() const {
    return (max_x - min_x) * (max_y - min_y);
}

Sensor::Sensor(SensorConfig config, Rng rng) : config_(config), rng_(std::move(rng)) {}

SensorReturn Sensor::observe(const std::vector<TruthTarget>& targets) {
    SensorReturn out;
    addTargetDetections(targets, out);
    addClutter(out);
    shuffleReturn(out);
    return out;
}

void Sensor::addTargetDetections(const std::vector<TruthTarget>& targets, SensorReturn& out) {
    for (const TruthTarget& target : targets) {
        if (!config_.coverage.contains(target.state.position)) {
            continue;
        }
        if (!rng_.bernoulli(config_.detection_probability)) {
            continue;
        }
        const double noise_x = rng_.gaussian(0.0, config_.position_noise_stddev);
        const double noise_y = rng_.gaussian(0.0, config_.position_noise_stddev);
        out.detections.push_back({target.state.position + Eigen::Vector2d{noise_x, noise_y}});
        out.origins.push_back(target.id);
    }
}

void Sensor::addClutter(SensorReturn& out) {
    const Area& area = config_.coverage;
    const int count = rng_.poisson(config_.clutter_rate);
    for (int i = 0; i < count; ++i) {
        const double x = rng_.uniform(area.min_x, area.max_x);
        const double y = rng_.uniform(area.min_y, area.max_y);
        out.detections.push_back({{x, y}});
        out.origins.push_back(kClutterOrigin);
    }
}

void Sensor::shuffleReturn(SensorReturn& out) {
    std::vector<std::size_t> order(out.detections.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    shuffle(order, rng_);

    SensorReturn shuffled;
    shuffled.detections.reserve(order.size());
    shuffled.origins.reserve(order.size());
    for (const std::size_t index : order) {
        shuffled.detections.push_back(out.detections[index]);
        shuffled.origins.push_back(out.origins[index]);
    }
    out = std::move(shuffled);
}

}  // namespace mtt::sim
