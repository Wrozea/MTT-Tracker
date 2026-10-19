#pragma once

#include <Eigen/Core>

namespace mtt::sim {

struct KinematicState {
    Eigen::Vector2d position;  // meters
    Eigen::Vector2d velocity;  // meters per second
};

// Moves a target at constant speed and constant turn rate for dt seconds.
// turn_rate is in radians per second; positive turns counterclockwise (left).
// The result is exact for any dt, so many short steps land in the same place
// as one long step.
KinematicState propagateConstantTurn(const KinematicState& state, double turn_rate, double dt);

}  // namespace mtt::sim
