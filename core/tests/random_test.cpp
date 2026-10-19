#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

#include "mtt/random.hpp"

namespace {

constexpr int kSamples = 100000;

struct SampleStats {
    double mean;
    double variance;
};

template <typename Draw>
SampleStats sampleStats(Draw draw) {
    std::vector<double> values(kSamples);
    for (double& value : values) {
        value = draw();
    }
    const double mean = std::accumulate(values.begin(), values.end(), 0.0) / kSamples;
    double squared_error = 0.0;
    for (const double value : values) {
        squared_error += (value - mean) * (value - mean);
    }
    return {mean, squared_error / (kSamples - 1)};
}

}  // namespace

// The C++ standard requires this exact value. It is the guarantee all of our
// determinism is built on.
TEST(Rng, Mt19937MatchesStandardRequiredValue) {
    std::mt19937 engine;
    engine.discard(9999);
    EXPECT_EQ(engine(), 4123659995u);
}

TEST(Rng, SameSeedAndStreamGiveSameSequence) {
    mtt::Rng a(42, 3);
    mtt::Rng b(42, 3);
    for (int i = 0; i < 1000; ++i) {
        ASSERT_EQ(a.nextU32(), b.nextU32());
    }
}

TEST(Rng, DifferentStreamsGiveDifferentSequences) {
    mtt::Rng a(42, 0);
    mtt::Rng b(42, 1);
    int matches = 0;
    for (int i = 0; i < 1000; ++i) {
        matches += a.nextU32() == b.nextU32() ? 1 : 0;
    }
    EXPECT_LT(matches, 5);
}

TEST(Rng, Uniform01StaysInHalfOpenRange) {
    mtt::Rng rng(1);
    for (int i = 0; i < kSamples; ++i) {
        const double value = rng.uniform01();
        ASSERT_GE(value, 0.0);
        ASSERT_LT(value, 1.0);
    }
}

TEST(Rng, UniformHasExpectedMeanAndVariance) {
    mtt::Rng rng(2);
    const SampleStats stats = sampleStats([&] { return rng.uniform(-3.0, 5.0); });
    // A uniform distribution on [a, b] has mean (a + b) / 2 and variance (b - a)^2 / 12.
    EXPECT_NEAR(stats.mean, 1.0, 0.05);
    EXPECT_NEAR(stats.variance, 64.0 / 12.0, 0.1);
}

TEST(Rng, UniformIntCoversInclusiveRangeAndNothingElse) {
    mtt::Rng rng(3);
    std::set<int> seen;
    for (int i = 0; i < 10000; ++i) {
        const int value = rng.uniformInt(-2, 4);
        ASSERT_GE(value, -2);
        ASSERT_LE(value, 4);
        seen.insert(value);
    }
    EXPECT_EQ(seen.size(), 7u);
}

TEST(Rng, UniformIntHandlesSingleValueAndFullRange) {
    mtt::Rng rng(4);
    EXPECT_EQ(rng.uniformInt(7, 7), 7);
    const int low = std::numeric_limits<int>::min();
    const int high = std::numeric_limits<int>::max();
    for (int i = 0; i < 100; ++i) {
        const int value = rng.uniformInt(low, high);
        EXPECT_GE(value, low);
        EXPECT_LE(value, high);
    }
}

TEST(Rng, UniformIntRejectsReversedBounds) {
    mtt::Rng rng(5);
    EXPECT_THROW(rng.uniformInt(3, 2), std::invalid_argument);
}

TEST(Rng, BernoulliMatchesProbability) {
    mtt::Rng rng(6);
    EXPECT_FALSE(rng.bernoulli(0.0));
    EXPECT_TRUE(rng.bernoulli(1.0));
    const SampleStats stats = sampleStats([&] { return rng.bernoulli(0.3) ? 1.0 : 0.0; });
    EXPECT_NEAR(stats.mean, 0.3, 0.01);
}

TEST(Rng, GaussianHasExpectedMeanAndStddev) {
    mtt::Rng rng(7);
    const SampleStats stats = sampleStats([&] { return rng.gaussian(10.0, 2.0); });
    EXPECT_NEAR(stats.mean, 10.0, 0.05);
    EXPECT_NEAR(std::sqrt(stats.variance), 2.0, 0.05);
}

TEST(Rng, GaussianTailsMatchNormalDistribution) {
    mtt::Rng rng(8);
    int beyond_two_sigma = 0;
    for (int i = 0; i < kSamples; ++i) {
        beyond_two_sigma += std::abs(rng.gaussian(0.0, 1.0)) > 2.0 ? 1 : 0;
    }
    // About 4.55% of a normal distribution lies more than 2 sigma from the mean.
    EXPECT_NEAR(static_cast<double>(beyond_two_sigma) / kSamples, 0.0455, 0.003);
}

TEST(Rng, PoissonMeanAndVarianceBothEqualRate) {
    mtt::Rng rng(9);
    const SampleStats stats = sampleStats([&] { return static_cast<double>(rng.poisson(4.5)); });
    EXPECT_NEAR(stats.mean, 4.5, 0.05);
    EXPECT_NEAR(stats.variance, 4.5, 0.1);
}

TEST(Rng, PoissonWithZeroMeanIsAlwaysZero) {
    mtt::Rng rng(10);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng.poisson(0.0), 0);
    }
}

TEST(Rng, PoissonRejectsUnsupportedMeans) {
    mtt::Rng rng(11);
    EXPECT_THROW(rng.poisson(-1.0), std::invalid_argument);
    EXPECT_THROW(rng.poisson(mtt::Rng::kMaxPoissonMean + 1.0), std::invalid_argument);
}

TEST(Shuffle, ProducesPermutationOfInput) {
    mtt::Rng rng(12);
    std::vector<int> items(50);
    std::iota(items.begin(), items.end(), 0);
    const std::vector<int> original = items;

    mtt::shuffle(items, rng);

    EXPECT_NE(items, original);
    std::sort(items.begin(), items.end());
    EXPECT_EQ(items, original);
}

TEST(Shuffle, PutsEachItemInEachPositionEvenly) {
    mtt::Rng rng(13);
    constexpr int kTrials = 30000;
    std::vector<int> first_position_counts(3, 0);
    for (int trial = 0; trial < kTrials; ++trial) {
        std::vector<int> items{0, 1, 2};
        mtt::shuffle(items, rng);
        ++first_position_counts[static_cast<std::size_t>(items[0])];
    }
    for (const int count : first_position_counts) {
        EXPECT_NEAR(static_cast<double>(count) / kTrials, 1.0 / 3.0, 0.015);
    }
}
