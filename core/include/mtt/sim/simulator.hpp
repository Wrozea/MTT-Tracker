#pragma once

#include <cstdint>
#include <vector>

#include "mtt/sim/scenario.hpp"
#include "mtt/sim/sensor.hpp"

namespace mtt::sim {

struct SimulatorConfig {
    double frame_period = 0.05;  // seconds, 20 Hz
    std::uint64_t seed = 1;
    SensorConfig sensor;
};

struct SimFrame {
    int index;
    double time;  // seconds since the start of the run
    std::vector<TruthTarget> truth;
    SensorReturn sensor;
};

// Runs a scenario one frame at a time. Each step reports where every live
// target truly is, then what the sensor saw, then advances the targets by one
// frame period. A target lives from its spawn time until its despawn time or
// until it leaves the sensor's coverage area, whichever comes first.
class Simulator {
public:
    Simulator(Scenario scenario, SimulatorConfig config);

    SimFrame step();

private:
    enum class Phase { Pending, Active, Retired };

    // Refers to its spec by index rather than pointer so that copying a
    // Simulator cannot leave the copy pointing into the original's scenario.
    struct TargetRuntime {
        std::size_t spec_index = 0;
        Phase phase = Phase::Pending;
        KinematicState state{};
        double turn_rate = 0.0;
        std::size_t next_turn = 0;
    };

    const TargetSpec& specOf(const TargetRuntime& target) const;
    void updateLifecycle(TargetRuntime& target, double time) const;
    void applyDueTurns(TargetRuntime& target, double time) const;
    std::vector<TruthTarget> activeTruth() const;
    void advanceTargets();

    Scenario scenario_;
    SimulatorConfig config_;
    Sensor sensor_;
    std::vector<TargetRuntime> targets_;
    int frame_index_ = 0;
};

}  // namespace mtt::sim
