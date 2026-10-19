#pragma once

#include <limits>
#include <vector>

#include "mtt/sim/motion.hpp"

namespace mtt::sim {

// From start_time on, the target turns at turn_rate (rad/s, positive is
// counterclockwise) until the next command. A turn_rate of 0 flies straight.
struct TurnCommand {
    double start_time;
    double turn_rate;
};

// A scripted target. Times are in seconds and take effect on the first frame
// at or after them.
struct TargetSpec {
    int id;
    double spawn_time;
    double despawn_time = std::numeric_limits<double>::infinity();
    KinematicState initial;
    std::vector<TurnCommand> turns;  // sorted by start_time
};

struct Scenario {
    std::vector<TargetSpec> targets;
};

struct TruthTarget {
    int id;
    KinematicState state;
};

// Six targets in a 1000 m x 1000 m area centered on the origin. Covers the
// hard cases for a tracker: a shallow-angle crossing, a right-angle crossing,
// a 180-degree turn, and targets that appear and disappear mid-run.
// docs/simulator.md describes each target.
Scenario makeDemoScenario();

}  // namespace mtt::sim
