# CLAUDE.md

Working rules for this repository. Read this before making changes, then read
`docs/progress.md` for the current status and what to do next.

## Project

A real-time multi-target tracking system built as a learning and portfolio
project. A C++ core simulates noisy sensor detections, tracks targets with a
Kalman filter and Hungarian data association, computes metrics against ground
truth, and streams one JSON message per frame over WebSocket to a React
dashboard.

The owner is a beginner who must be able to explain every line in an
interview. Favor clarity over cleverness, and explain decisions.

## Layout

- `core/` C++ tracker, simulator, and server. Its `CMakeLists.txt` is the
  project root for all C++ code.
- `core/tests/` GoogleTest tests.
- `dashboard/` React + TypeScript + Vite app.
- `docs/` Architecture notes and design decisions. `docs/decisions.md` is the
  running log of decisions; add to it when a choice is made.
  `docs/progress.md` holds current status and the next step.

## Fixed tech choices

Do not substitute any of these without asking the owner first.

- C++20, CMake, Eigen, GoogleTest, IXWebSocket, nlohmann/json.
- Dependencies come from CMake `FetchContent`, pinned to a release tag or
  commit hash and verified with a SHA256 hash. Never pin to a branch.
- IXWebSocket is built with `USE_TLS=OFF` and `USE_ZLIB=OFF`.
- Kalman filter with a 2D constant-velocity model (x, y, vx, vy).
- Gating plus a hand-written Hungarian algorithm for association.
- M-of-N track confirmation, deletion after too many misses.
- Metrics follow CLEAR MOT (Bernardin and Stiefelhagen, 2008): MOTA, MOTP, ID
  switches, plus position RMSE and false track count. Do not invent a metric.
- Dashboard: React + TypeScript + Vite, drawn on an HTML canvas.
- Docker Compose for one-command runs, GitHub Actions CI on Ubuntu and Windows
  (Release, warnings as errors), plus an Ubuntu Debug job with ASan and UBSan.

## Scope decisions

- Detections are Cartesian x/y, so the Kalman filter stays linear. Range and
  bearing are out of scope; the known-limitations section lists an EKF and
  converted measurements as possible next steps.
- The filter is checked for consistency with NEES and NIS, and the measured
  results (including lag on turning targets) are reported in known
  limitations.
- A headless mode (for example `tracker --headless --seed 42 --frames 2000`)
  runs a fixed-seed scenario, prints metrics, and exits. CI runs it and
  compares the metrics to stored values within a tolerance. README metric
  numbers come from this run.
- An IMM (interacting multiple model) filter for turning targets is a stretch
  goal only. Do not build it unless the owner asks after the main milestones
  are done.

## Determinism rules

- All randomness goes through `mt19937` plus our own distribution code. Never
  use `std::*_distribution`; their output is implementation-defined.
- Cross-platform results are near-identical, not bit-identical (libm and
  floating-point contraction can differ in the last bit). Any CI comparison of
  metrics uses a tolerance.
- Commands from the dashboard (reset, seed change, settings) are applied only
  at frame boundaries. Outgoing messages are built from a snapshot of the
  completed frame, never from live state.

## Workflow

- Work in small milestones and wait for the owner's approval before starting
  each one.
- After each milestone: build, run all tests, show the output, explain in
  plain English what was built, how it works, and why each decision was made,
  then suggest a commit message. The owner makes the commits.
- At the end of every milestone, append that milestone's section to
  `docs/learning-notes.md`: the concepts to understand (short plain-English
  explanations), the bugs hit and what caused them, and likely interview
  questions with short answers. Bugs found after a milestone closes go into
  that milestone's section. The file is gitignored and stays on the owner's
  machine; create it if it is missing, and never remove it from
  `.gitignore`.
- Update `docs/progress.md` at the end of every milestone: status table,
  what exists, the plan for the next milestone, and any new environment
  notes.
- Back up the learning notes: copy `docs/learning-notes.md` to
  `C:\Users\18155\Documents\mtt-notes\learning-notes.md` (a clone of the
  private repo `Wrozea/mtt-notes`), commit there, and push. This is the one
  place Claude commits and pushes without being asked each time; commits to
  this repository are still made by the owner. The GitHub CLI is at
  `C:\Program Files\GitHub CLI\gh.exe` and is not on PATH.
- If something in the spec looks like a bad idea, say so before doing it.
- For the Kalman filter and the Hungarian algorithm, explain the idea and the
  math before writing any code, then offer to let the owner try implementing
  it first. Write it only if the owner declines, and review their version if
  they accept.

## Code style

- Clear names, small functions, professional and readable.
- Comments only where the "why" is not obvious. No filler comments.
- No emojis in code or docs. No ASCII banners.
- Every algorithm gets unit tests.
- Never store an Eigen expression in `auto`. Write the type
  (`Eigen::Vector2d`, `Eigen::Matrix4d`) so the result is computed
  immediately; `auto` keeps an unevaluated expression that references its
  operands, which dangles if an operand is a temporary.

## Build and test

From the repository root:

    cmake -S core -B build
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure

With AddressSanitizer (run this before declaring a milestone done):

    cmake -S core -B build-asan -DMTT_ENABLE_ASAN=ON
    cmake --build build-asan --config Debug
    ctest --test-dir build-asan -C Debug --output-on-failure

The local machine has only MSVC. GCC-specific warnings and behavior are
checked only by CI on Ubuntu.
