#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "mtt/sim/scenario.hpp"
#include "mtt/sim/sensor.hpp"
#include "mtt/sim/simulator.hpp"

namespace {

struct Options {
    std::uint64_t seed = 1;
    int frames = 1200;
    int print_every = 20;
};

void printUsage() {
    std::cerr << "usage: tracker [--seed N] [--frames N] [--print-every N]\n";
}

std::optional<Options> parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view flag = argv[i];
        if (i + 1 >= argc) {
            return std::nullopt;
        }
        const std::string value = argv[++i];
        try {
            if (flag == "--seed") {
                options.seed = std::stoull(value);
            } else if (flag == "--frames") {
                options.frames = std::stoi(value);
            } else if (flag == "--print-every") {
                options.print_every = std::max(1, std::stoi(value));
            } else {
                return std::nullopt;
            }
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    return options;
}

void printFrame(const mtt::sim::SimFrame& frame) {
    const auto& origins = frame.sensor.origins;
    const auto clutter = std::count(origins.begin(), origins.end(), mtt::sim::kClutterOrigin);
    std::printf("t=%6.2f s  frame %5d  targets %zu  detections %zu (%td from targets, %td clutter)\n",
                frame.time, frame.index, frame.truth.size(), origins.size(),
                static_cast<std::ptrdiff_t>(origins.size()) - clutter, clutter);
    for (const auto& target : frame.truth) {
        std::printf("    target %d  pos (%7.1f, %7.1f)  vel (%6.1f, %6.1f)\n", target.id,
                    target.state.position.x(), target.state.position.y(),
                    target.state.velocity.x(), target.state.velocity.y());
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::optional<Options> options = parseOptions(argc, argv);
    if (!options) {
        printUsage();
        return EXIT_FAILURE;
    }

    mtt::sim::SimulatorConfig config;
    config.seed = options->seed;
    mtt::sim::Simulator simulator(mtt::sim::makeDemoScenario(), config);

    for (int i = 0; i < options->frames; ++i) {
        const mtt::sim::SimFrame frame = simulator.step();
        if (frame.index % options->print_every == 0) {
            printFrame(frame);
        }
    }
    return EXIT_SUCCESS;
}
