#include "mtt/sim/simulator.hpp"

#include <utility>

namespace mtt::sim {

namespace {

// Each part of the simulation draws from its own stream, so changing one part
// (for example the clutter rate) never changes the random numbers another
// part receives.
constexpr std::uint64_t kSensorStream = 1;

// Frame times are computed as index * period, which is not exact in binary
// floating point (20 * 0.05 may come out as 0.9999999999999999). The epsilon
// makes an event scheduled at 1.0 s fire on frame 20, not frame 21.
constexpr double kTimeEpsilon = 1e-9;

bool hasReached(double time, double event_time) {
    return time + kTimeEpsilon >= event_time;
}

}  // namespace

Simulator::Simulator(Scenario scenario, SimulatorConfig config)
    : scenario_(std::move(scenario)),
      config_(config),
      sensor_(config.sensor, Rng(config.seed, kSensorStream)) {
    targets_.resize(scenario_.targets.size());
    for (std::size_t i = 0; i < targets_.size(); ++i) {
        targets_[i].spec_index = i;
    }
}

SimFrame Simulator::step() {
    const double time = frame_index_ * config_.frame_period;

    for (TargetRuntime& target : targets_) {
        updateLifecycle(target, time);
        if (target.phase == Phase::Active) {
            applyDueTurns(target, time);
        }
    }

    SimFrame frame{.index = frame_index_, .time = time, .truth = activeTruth(), .sensor = {}};
    frame.sensor = sensor_.observe(frame.truth);

    advanceTargets();
    ++frame_index_;
    return frame;
}

const TargetSpec& Simulator::specOf(const TargetRuntime& target) const {
    return scenario_.targets[target.spec_index];
}

void Simulator::updateLifecycle(TargetRuntime& target, double time) const {
    const TargetSpec& spec = specOf(target);
    if (target.phase == Phase::Pending && hasReached(time, spec.spawn_time)) {
        target.phase = Phase::Active;
        target.state = spec.initial;
    }
    if (target.phase == Phase::Active) {
        const bool expired = hasReached(time, spec.despawn_time);
        const bool left_coverage = !config_.sensor.coverage.contains(target.state.position);
        if (expired || left_coverage) {
            target.phase = Phase::Retired;
        }
    }
}

void Simulator::applyDueTurns(TargetRuntime& target, double time) const {
    const std::vector<TurnCommand>& turns = specOf(target).turns;
    while (target.next_turn < turns.size() && hasReached(time, turns[target.next_turn].start_time)) {
        target.turn_rate = turns[target.next_turn].turn_rate;
        ++target.next_turn;
    }
}

std::vector<TruthTarget> Simulator::activeTruth() const {
    std::vector<TruthTarget> truth;
    for (const TargetRuntime& target : targets_) {
        if (target.phase == Phase::Active) {
            truth.push_back({specOf(target).id, target.state});
        }
    }
    return truth;
}

void Simulator::advanceTargets() {
    for (TargetRuntime& target : targets_) {
        if (target.phase == Phase::Active) {
            target.state = propagateConstantTurn(target.state, target.turn_rate, config_.frame_period);
        }
    }
}

}  // namespace mtt::sim
