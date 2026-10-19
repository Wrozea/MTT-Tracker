#include "mtt/random.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace mtt {

namespace {

// SplitMix64 scrambles nearby inputs (seed 1 vs seed 2, stream 0 vs 1) into
// unrelated outputs. It is pure integer arithmetic, so it is portable.
std::uint64_t splitMix64(std::uint64_t value) {
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31);
}

std::uint32_t engineSeed(std::uint64_t seed, std::uint64_t stream) {
    const std::uint64_t mixed = splitMix64(splitMix64(seed) ^ stream);
    return static_cast<std::uint32_t>(mixed ^ (mixed >> 32));
}

}  // namespace

Rng::Rng(std::uint64_t seed, std::uint64_t stream) : engine_(engineSeed(seed, stream)) {}

std::uint32_t Rng::nextU32() {
    return static_cast<std::uint32_t>(engine_());
}

double Rng::uniform01() {
    // Two separate statements on purpose: inside one expression the order of
    // the two engine calls would be unspecified and could differ by compiler.
    const std::uint64_t high = nextU32() >> 5;  // 27 bits
    const std::uint64_t low = nextU32() >> 6;   // 26 bits
    const std::uint64_t bits53 = (high << 26) | low;
    return static_cast<double>(bits53) * 0x1.0p-53;
}

double Rng::uniform(double low, double high) {
    return low + (high - low) * uniform01();
}

int Rng::uniformInt(int low, int high) {
    if (high < low) {
        throw std::invalid_argument("uniformInt: high < low");
    }
    const std::uint64_t span = static_cast<std::uint64_t>(static_cast<std::int64_t>(high) - low) + 1;
    // Taking draw % span directly would favor small values whenever 2^32 is
    // not a multiple of span. Rejecting draws above the largest multiple of
    // span removes that bias.
    const std::uint64_t accept_below = ((std::uint64_t{1} << 32) / span) * span;
    std::uint64_t draw = nextU32();
    while (draw >= accept_below) {
        draw = nextU32();
    }
    return static_cast<int>(static_cast<std::int64_t>(low) + static_cast<std::int64_t>(draw % span));
}

bool Rng::bernoulli(double probability) {
    return uniform01() < probability;
}

double Rng::gaussian(double mean, double stddev) {
    // Box-Muller transform. 1 - u keeps the log argument in (0, 1], so log(0)
    // can never happen. The second value the transform could produce is
    // discarded to keep the generator stateless between calls.
    const double u1 = 1.0 - uniform01();
    const double u2 = uniform01();
    const double radius = std::sqrt(-2.0 * std::log(u1));
    return mean + stddev * radius * std::cos(2.0 * std::numbers::pi * u2);
}

int Rng::poisson(double mean) {
    if (mean < 0.0 || mean > kMaxPoissonMean) {
        throw std::invalid_argument("poisson: mean out of supported range");
    }
    // Knuth's method: multiply uniforms until the product falls to e^-mean or
    // below. The number of extra factors needed is Poisson distributed. Cost
    // grows with the mean, which is fine for clutter rates of a few dozen.
    const double threshold = std::exp(-mean);
    int count = 0;
    double product = uniform01();
    while (product > threshold) {
        ++count;
        product *= uniform01();
    }
    return count;
}

}  // namespace mtt
