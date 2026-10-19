#pragma once

#include <Eigen/Core>

namespace mtt {

// One sensor report: a measured position in meters. This is all the tracker
// is allowed to see.
struct Detection {
    Eigen::Vector2d position;
};

}  // namespace mtt
