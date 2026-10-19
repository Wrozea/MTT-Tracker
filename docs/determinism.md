# Determinism

A run is defined by its seed. The same seed must give the same targets, the
same noise, the same misses, and the same clutter, so that a bug can be
replayed, a tuning change can be compared fairly, and CI can check metrics
against stored values.

## What the C++ standard guarantees, and what it does not

| Piece | Specified by the standard? |
|---|---|
| `std::mt19937` output sequence for a given seed | Yes, exactly. The standard even fixes its 10000th output (4123659995), which `Rng.Mt19937MatchesStandardRequiredValue` checks. |
| `std::uniform_real_distribution`, `uniform_int_distribution`, `normal_distribution`, `poisson_distribution`, `bernoulli_distribution` | No. Only the resulting distribution is specified, not the algorithm, so MSVC, libstdc++ (GCC), and libc++ (Clang) turn the same engine output into different numbers. |
| `std::shuffle` | No. How it consumes engine output is up to the library. |
| `std::sin`, `std::cos`, `std::log`, `std::exp`, `std::sqrt` | `sqrt` must be correctly rounded. The others only need to be close; different standard libraries can differ in the last bit. |
| `a * b + c` | The compiler may fuse it into one FMA instruction, which rounds once instead of twice and gives a slightly different result. |

## What we do about it

1. **All randomness goes through `mtt::Rng`** (`core/include/mtt/random.hpp`).
   It wraps `std::mt19937` and implements every distribution itself:
   - Uniform `[0, 1)`: takes 27 + 26 bits from two engine calls and scales
     by 2^-53. Pure integer arithmetic plus one exact multiply, so it is
     bit-identical everywhere.
   - Uniform integer: rejection sampling, so no value is favored.
     Bit-identical everywhere.
   - Bernoulli: `uniform01() < p`. Bit-identical.
   - Gaussian: Box-Muller. Uses `log`, `sqrt`, and `cos`, so it can differ in
     the last bit across platforms.
   - Poisson: Knuth's multiplication method. Uses `exp` once per call.
   - Shuffle: Fisher-Yates on top of our uniform integer. Bit-identical.
2. **`std::*_distribution` and `std::shuffle` are never used.** This is a
   rule in `CLAUDE.md`.
3. **Two engine calls are never made inside one expression.**
   `(nextU32() << 26) | nextU32()` would be a bug: C++ does not specify
   which side runs first, so compilers may pick different orders.
4. **FMA contraction is turned off** with `-ffp-contract=off` on GCC and
   Clang. MSVC's default `/fp:precise` does not contract.
5. **Independent random streams.** `Rng(seed, stream)` derives a separate
   engine per subsystem by scrambling the seed and stream number with
   SplitMix64. The sensor uses stream 1. If one subsystem draws more numbers
   (for example a higher clutter rate), others are not affected.

## Near-identical, not bit-identical

Even with all of the above, `log`, `cos`, and `exp` can differ in the last bit
between MSVC and GCC. One different bit in one noise sample is harmless by
itself. But the tracker makes yes/no decisions (is this detection inside the
gate?), and a detection sitting exactly on the gate boundary could go
different ways on two platforms. That one choice can then change later frames.

So the policy is:

- **Same platform, same build:** results are bit-identical. Tests that compare
  two runs with the same seed use exact equality.
- **Across platforms:** results are near-identical. CI checks of headless-run
  metrics (MOTA, MOTP, RMSE, ID switches, false tracks) compare against stored
  values within a tolerance, never with exact equality.

## Things that would silently break determinism

Watch for these in code review:

- Seeding from the clock or `std::random_device`.
- Using any `std::*_distribution` or `std::shuffle`.
- Iterating over `std::unordered_map` or `std::unordered_set` when the order
  affects results. Their order depends on the library and on hashing.
- Sorting with ties in a way the result depends on, when the sort is not
  stable.
- Running the simulation or tracker step on several threads where the order
  of floating-point additions depends on timing.
- Applying dashboard commands (reset, seed change, settings) in the middle of
  a step. Commands are applied only at frame boundaries.
