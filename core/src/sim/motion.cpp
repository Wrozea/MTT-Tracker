#include "mtt/sim/motion.hpp"

#include <cmath>

namespace mtt::sim {

namespace {

// Below this, the turning formula divides by a number close to zero and loses
// precision, while straight-line motion is accurate to well under a nanometer.
constexpr double kStraightLineTurnRate = 1e-9;

}  // namespace

KinematicState propagateConstantTurn(const KinematicState& state, double turn_rate, double dt) {
    const double vx = state.velocity.x();
    const double vy = state.velocity.y();

    if (std::abs(turn_rate) < kStraightLineTurnRate) {
        return {state.position + state.velocity * dt, state.velocity};
    }

    // Integrating a velocity vector that rotates at turn_rate gives these
    // closed-form expressions for the displacement along the circular arc.
    const double sin_turn = std::sin(turn_rate * dt);
    const double cos_turn = std::cos(turn_rate * dt);
    const Eigen::Vector2d displacement{
        (vx * sin_turn - vy * (1.0 - cos_turn)) / turn_rate,
        (vx * (1.0 - cos_turn) + vy * sin_turn) / turn_rate};
    const Eigen::Vector2d new_velocity{
        vx * cos_turn - vy * sin_turn,
        vx * sin_turn + vy * cos_turn};

    return {state.position + displacement, new_velocity};
}

}  // namespace mtt::sim
