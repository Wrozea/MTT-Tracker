#include "mtt/sim/scenario.hpp"

#include <numbers>

namespace mtt::sim {

Scenario makeDemoScenario() {
    constexpr double kHalfTurnOver16s = std::numbers::pi / 16.0;

    Scenario scenario;
    // Targets 1 and 4 cross at a right angle at (0, -300) at t = 30 s.
    scenario.targets.push_back({.id = 1,
                                .spawn_time = 0.0,
                                .initial = {{-450.0, -300.0}, {15.0, 0.0}},
                                .turns = {}});
    // Targets 2 and 3 converge at a shallow angle and pass through the same
    // point, (-200, 50), at t = 10 s. This is the classic case where a weak
    // tracker swaps their IDs.
    scenario.targets.push_back({.id = 2,
                                .spawn_time = 0.0,
                                .initial = {{-400.0, 100.0}, {20.0, -5.0}},
                                .turns = {}});
    scenario.targets.push_back({.id = 3,
                                .spawn_time = 0.0,
                                .initial = {{-400.0, 0.0}, {20.0, 5.0}},
                                .turns = {}});
    scenario.targets.push_back({.id = 4,
                                .spawn_time = 0.0,
                                .initial = {{0.0, 300.0}, {0.0, -20.0}},
                                .turns = {}});
    // Target 5 appears at t = 5 s heading north, makes a 180-degree left turn
    // from t = 10 s to t = 26 s, then flies south out of the area.
    scenario.targets.push_back({.id = 5,
                                .spawn_time = 5.0,
                                .initial = {{300.0, -400.0}, {0.0, 20.0}},
                                .turns = {{10.0, kHalfTurnOver16s}, {26.0, 0.0}}});
    // Target 6 appears at t = 20 s and disappears at t = 50 s while still
    // inside the area.
    scenario.targets.push_back({.id = 6,
                                .spawn_time = 20.0,
                                .despawn_time = 50.0,
                                .initial = {{400.0, 400.0}, {-15.0, -10.0}},
                                .turns = {}});
    return scenario;
}

}  // namespace mtt::sim
