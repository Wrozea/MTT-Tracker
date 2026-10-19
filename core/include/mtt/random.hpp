#pragma once

#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace mtt {

// Random number source whose output depends only on the seed, on every
// compiler. The C++ standard fixes the exact sequence std::mt19937 produces,
// but not the algorithms behind std::*_distribution, so MSVC and GCC turn the
// same engine output into different numbers. Every distribution here is our
// own code on top of the raw engine output. See docs/determinism.md.
class Rng {
public:
    // Different stream values give independent sequences from the same seed,
    // so one part of the simulation can draw more or fewer numbers without
    // shifting the numbers another part sees.
    explicit Rng(std::uint64_t seed, std::uint64_t stream = 0);

    std::uint32_t nextU32();

    // Uniform in [0, 1) using 53 random bits, the full precision of a double.
    double uniform01();
    double uniform(double low, double high);
    // Uniform integer in [low, high], both ends included.
    int uniformInt(int low, int high);
    bool bernoulli(double probability);
    double gaussian(double mean, double stddev);
    // Supports means up to kMaxPoissonMean; throws above that.
    int poisson(double mean);

    static constexpr double kMaxPoissonMean = 500.0;

private:
    std::mt19937 engine_;
};

// Fisher-Yates shuffle. std::shuffle is not used because, like the
// distributions, its use of the engine is implementation-defined.
template <typename T>
void shuffle(std::vector<T>& items, Rng& rng) {
    for (int i = static_cast<int>(items.size()) - 1; i > 0; --i) {
        const int j = rng.uniformInt(0, i);
        std::swap(items[static_cast<std::size_t>(i)], items[static_cast<std::size_t>(j)]);
    }
}

}  // namespace mtt
