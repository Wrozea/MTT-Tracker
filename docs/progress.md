# Progress

Current status of the project. Read this at the start of every session, after
`CLAUDE.md`. Update it at the end of every milestone.

Last updated: 2026-10-19

## Status

Milestones 1 and 2 are done, committed, and pushed. CI is green on all three
jobs: Ubuntu Release, Windows Release, and Ubuntu Debug with ASan + UBSan.
37 unit tests pass locally in Release and under ASan.

**Next: milestone 3, the Kalman filter. Not started.**

## Milestones

| # | Milestone | Status |
|---|---|---|
| 1 | Skeleton, CMake, one test, CI | Done |
| 2 | Simulator with ground truth | Done |
| 3 | Kalman filter, one target, with tests | **Next** |
| 4 | Gating + Hungarian association | Not started |
| 5 | Track management (tentative / confirmed / deleted) | Not started |
| 6 | Metrics (CLEAR MOT, RMSE, false tracks) + headless mode | Not started |
| 7 | WebSocket server streaming JSON | Not started |
| 8 | Dashboard | Not started |
| 9 | Controls, Docker, README polish | Not started |

## What exists

- `core/include/mtt/random.hpp`: `mtt::Rng`, own distributions on top of
  `mt19937` (uniform, integer, Bernoulli, Gaussian, Poisson, shuffle).
- `core/include/mtt/types.hpp`: `mtt::Detection`, the only thing the tracker
  receives.
- `core/include/mtt/sim/`: constant-turn motion, scripted scenarios
  (`makeDemoScenario()`), sensor model, `Simulator` (20 Hz, 0.05 s frames).
- `core/apps/tracker_main.cpp`: prints simulator frames
  (`--seed`, `--frames`, `--print-every`). No tracker yet.
- `core/cmake/`: warnings, sanitizers (`MTT_ENABLE_ASAN`, `MTT_ENABLE_UBSAN`),
  and pinned, SHA256-verified dependencies (GoogleTest 1.17.0, Eigen 3.4.0).
  IXWebSocket and nlohmann/json are not added yet; add them, pinned the same
  way, when milestone 7 needs them.
- `docs/`: `decisions.md` (running decision log), `determinism.md`,
  `simulator.md`, `progress.md` (this file). `learning-notes.md` is local only
  (gitignored, backed up to the private repo `Wrozea/mtt-notes`).

## Milestone 3 plan: Kalman filter

The owner implements this one. Order of work:

1. **Explain first, no code.** The idea (predict, then correct with a
   measurement, weighting each by its uncertainty) and the math for our
   constant-velocity model:
   - state `x = [px, py, vx, vy]`, covariance `P`
   - transition matrix `F` for time step `dt`
   - process noise `Q` (discrete white-noise acceleration model, tuned by
     one acceleration variance)
   - measurement matrix `H` (we measure position only), measurement noise
     `R` (5 m standard deviation, matching the sensor)
   - predict: `x = F x`, `P = F P F^T + Q`
   - update: innovation `y = z - H x`, innovation covariance
     `S = H P H^T + R`, gain `K = P H^T S^-1`, `x = x + K y`,
     `P = (I - K H) P` (or the Joseph form, and why it is more stable)
   - initialization from a single detection (velocity unknown, so large
     velocity variance)
   - NEES and NIS: what they measure, their chi-square expected values
     (4 and 2), and how they reveal an over- or under-confident filter
2. **Offer the owner the implementation** with a suggested interface and a
   list of tests to write. Review their code. Write it only if they ask.
3. **Tests**, at least: prediction moves the state by `v * dt`; covariance
   grows on predict and shrinks on update; a stationary target converges;
   a straight-line target converges in position and velocity; filtered error
   is lower than raw detection error; average NEES and NIS on a straight
   target fall inside their chi-square bounds.
4. **Console demo**: track one simulator target (no clutter, no association
   yet) and print truth, detection, and estimate.
5. **Close the milestone**: build, test (Release and ASan), explain, update
   `decisions.md`, append the milestone 3 section to `learning-notes.md` and
   back it up, update this file, suggest a commit message.

Expected finding to report later: the constant-velocity filter lags on
target 5's turn, and NEES rises during it. That goes into the README's known
limitations, not into a fix (an IMM is a stretch goal only).

## Environment notes for a fresh session

- Windows 11, PowerShell plus Git Bash. Project at
  `C:\Users\18155\projects\mtt-tracker`, remote `Wrozea/MTT-Tracker`.
- Compiler: MSVC from VS Build Tools 2026. `cl` is not on PATH; the Visual
  Studio generator finds it on its own. CMake is the copy bundled with Build
  Tools and must be put on PATH per command:
  `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`
- Build folders: `build/` (Release) and `build-asan/` (Debug + ASan).
- No GCC, WSL, or Docker locally. GCC-only warnings and bugs show up only in
  CI. All three GCC failures so far (missing field initializers, dangling
  reference, Eigen `auto` use-after-free) were caught there. Run the ASan
  build before calling a milestone done. WSL plus Docker Desktop must be
  installed by the owner before milestone 9.
- GitHub CLI: `C:\Program Files\GitHub CLI\gh.exe`, not on PATH, logged in
  as `Wrozea`. Useful for checking CI: `gh run list --repo Wrozea/MTT-Tracker`.
- The owner commits and pushes `mtt-tracker`. Claude only commits and pushes
  the notes backup in `C:\Users\18155\Documents\mtt-notes`.

## Open items

- Random scenario generator: deferred to milestone 9 (target-count control).
- README GIF: the owner records it in milestone 9 (ScreenToGif).
