# Design decisions

A running log of decisions and the reasons behind them. Topics that need more
than a paragraph get their own file in this folder.

## Build and dependencies

- `core/` is a standalone CMake project. The C++ code builds the same way on a
  laptop, in CI, and inside the Docker image, without depending on the
  dashboard.
- Dependencies are fetched by CMake `FetchContent` from pinned release
  archives, each verified by SHA256. This needs no package manager and fails
  loudly if an upstream archive changes.
- IXWebSocket is built with `USE_TLS=OFF` and `USE_ZLIB=OFF`. The server runs
  on localhost or inside Docker, so TLS and compression would only add OpenSSL
  and zlib as dependencies.
- JSON serialization uses nlohmann/json.
- CI builds with warnings as errors on Ubuntu (GCC) and Windows (MSVC). Two
  compilers catch non-portable code that one alone would accept.
- `MTT_ENABLE_ASAN=ON` builds with AddressSanitizer (MSVC `/fsanitize=address`,
  GCC/Clang `-fsanitize=address`). It caught a use-after-free in a sensor test
  that read garbage on GCC and happened to work on MSVC: an `auto` variable
  held an unevaluated Eigen expression pointing into a temporary. Eigen
  results are therefore always stored in an explicit type, never `auto`.
- CI has a third job: Ubuntu Debug with ASan and UBSan
  (`MTT_ENABLE_UBSAN=ON`, GCC/Clang only). It runs the tests and the full demo
  scenario. UBSan is built with `-fno-sanitize-recover` so any report fails
  the job; by default it would only print a message. This job does not treat
  warnings as errors, because sanitizer instrumentation can produce
  false-positive GCC warnings and the Release jobs already enforce warnings.
- `CMAKE_CXX_EXTENSIONS OFF` builds in strict ISO C++ mode for portability.
- GCC and Clang build with `-ffp-contract=off`, so they never fuse
  `a * b + c` into one fused multiply-add instruction. A fused operation
  rounds once instead of twice, so whether the compiler fuses changes the
  result. MSVC's default `/fp:precise` does not fuse, so this keeps the
  compilers closer together.

## Determinism

- Runs are seeded and repeatable. All randomness uses `std::mt19937`, whose
  output sequence is fixed by the C++ standard, plus distribution code we
  write ourselves (Gaussian, uniform, Poisson, Bernoulli). The standard
  library distributions are implementation-defined and give different values
  on MSVC and GCC for the same seed.
- Cross-platform results are treated as near-identical, not bit-identical.
  `std::log`, `std::sin`, and `std::cos` can differ in the last bit between
  standard libraries, and those differences can grow over a long run. CI
  metric checks therefore compare with a tolerance. Details are in
  `docs/determinism.md`.
- Each subsystem draws from its own random stream, so raising the clutter
  rate does not change the noise on target detections.

## Simulator

- Ground truth uses an exact constant-turn model, so a target's path does not
  depend on the frame rate. Details are in `docs/simulator.md`.
- Ground truth follows its script with no random motion. Scripted motion
  makes the hard cases (crossings, turns) happen exactly where and when the
  scenario says, so tests can check them.
- A target is retired once it leaves sensor coverage. Otherwise metrics would
  count the tracker as failing on targets nobody can see.
- Sensor output is shuffled so that list order cannot leak which detections
  are real.
- Each detection's origin is recorded for the dashboard and tests, kept beside
  the detections rather than inside `Detection`, so the tracker cannot read
  it by accident.

## Tracking

- Constant-velocity Kalman filter. It lags on turning targets; we measure how
  much with NEES and NIS consistency checks instead of only describing it.
- Measurements are Cartesian x/y, which keeps the filter linear. Range and
  bearing measurements would need either an EKF or converted measurements.
- An IMM filter, which mixes a straight-line and a turning motion model, is
  the standard fix for turn lag. It is a stretch goal, not part of the core
  milestones.

## Metrics

- CLEAR MOT (Bernardin and Stiefelhagen, 2008): MOTA, MOTP, and ID switches,
  plus position RMSE and false track count. Matches from the previous frame
  are kept while still inside the distance gate; Hungarian runs only on the
  rest.
- A headless mode runs a fixed-seed scenario and prints metrics. CI compares
  them to stored values within a tolerance.

## Runtime

- Dashboard commands go through a thread-safe queue and are applied only at
  frame boundaries, so a reset or seed change never happens mid-step.
- Each outgoing message is built from a snapshot of the completed frame,
  never from live state that the next step might be changing.
