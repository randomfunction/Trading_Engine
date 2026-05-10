# Small Low-Latency Matching Engine

A compact C++17 matching engine prototype for low-latency systems practice.

The project is intentionally small in scope:

- one process
- one symbol
- one matching thread
- one SPSC queue
- one replay / benchmark driver
- one bounded in-memory order book

The goal is not to imitate a full exchange. The goal is to build a small system that is deeply understood, mechanically sympathetic, and benchmarked carefully.

## Why This Shape

For a student or new-grad systems project, breadth is often less convincing than depth.

This repository focuses on a few high-signal ideas and implements them cleanly:

- deterministic price-time matching
- single-writer ownership of mutable book state
- lock-free SPSC queue between producer and matcher
- fixed-size object pool for order storage
- cache-aware price ladder instead of tree-based containers
- replayable benchmark input
- bounded memory and allocator avoidance in the hot path

Everything here should be explainable line-by-line in an interview.

## What It Is

This is a single-host prototype designed to answer a narrow question:

What does a small but technically serious low-latency matching engine look like when built by one person with attention to data layout, ownership, and measurement?

It is not:

- a distributed system
- a fake exchange platform
- a market-data infrastructure project
- a persistence or journaling project
- a production claim

## Final Architecture

The system is intentionally minimal.

```text
replay / benchmark driver
          |
          v
   SPSC ring buffer
          |
          v
  single matching thread
          |
          v
  in-memory reports / counters
```

Design rules:

- the benchmark driver is the single producer
- the matching core is the single consumer
- only the matching thread mutates book state
- the queue is bounded and preallocated
- the order book uses preallocated storage
- replay input is totally ordered and deterministic

This keeps the concurrency model simple enough to reason about while still preserving real low-latency systems concepts.

## Core Components

### `SpscRing`

A bounded single-producer / single-consumer ring buffer used to feed commands into the matching thread.

Why it matters:

- no mutexes in the handoff path
- fixed capacity
- cache-line-separated producer and consumer indices
- simple enough to inspect and reason about completely

### `LadderBook`

A one-symbol in-memory order book with price-time-priority matching.

Current design:

- dense price ladder over a bounded price range
- direct price-to-index mapping
- FIFO per price level
- direct order-id lookup for cancel / modify
- intrusive links using indices rather than heap-allocated nodes

Why it matters:

- predictable access patterns
- avoids `std::map` in the hot path
- keeps traversal more contiguous and less pointer-heavy

### `ObjectPool`

A fixed-size pool used to store order nodes.

Why it matters:

- avoids heap allocation during steady-state matching
- bounds memory usage
- makes lifetime behavior explicit

### `MatchingEngine`

A small wrapper around the book that consumes ordered commands and emits deterministic reports.

Why it matters:

- separates command processing from the benchmark driver
- keeps single-writer ownership explicit
- makes replay and tests straightforward

## Mechanical Sympathy

This project keeps only the low-latency ideas that are high-signal and realistic at this size:

- cache-line padding on hot queue state
- producer / consumer index separation to reduce false sharing
- contiguous arrays for price levels and object storage
- bounded memory usage
- minimizing pointer chasing in book traversal
- avoiding allocator traffic after initialization
- deterministic event ordering
- simple branch structure in the common add / match paths

The point is not to use every optimization technique. The point is to choose a few that materially affect behavior and can be defended clearly.

## Repository Layout

```text
.
├── README.md
├── CMakeLists.txt
├── apps/
│   ├── bench_main.cpp
│   ├── replay_main.cpp
│   └── tests_main.cpp
├── include/llx/
│   ├── book/
│   ├── core/
│   ├── memory/
│   ├── metrics/
│   ├── queue/
│   └── util/
└── src/
    ├── book/
    └── core/
```

This layout is intentionally small. There is no infrastructure layer, deployment folder, or extra architecture scaffolding.

## Build

```bash
cmake -S . -B build-llx
cmake --build build-llx -j
```

Available binaries:

- `llx_tests`
- `llx_replay`
- `llx_bench`

## What The Binaries Do

### `llx_tests`

Small correctness checks for add, cancel, modify, and matching behavior.

### `llx_replay`

Runs a fixed deterministic command stream through the engine and prints a compact correctness summary.

### `llx_bench`

Runs a bounded replay-style benchmark:

- one producer thread pushes commands into the SPSC ring
- one matching thread consumes and processes them
- warmup traffic is sent first
- measured traffic is recorded separately
- throughput and prototype latency summaries are emitted

## Latest Verified Outputs

Verified locally on May 11, 2026 from the current `build-llx` tree.

### `llx_tests`

```bash
./build-llx/llx_tests
```

```text
llx_tests: passed
```

### `llx_replay`

```bash
./build-llx/llx_replay
```

```text
Replay complete
Trades: 2
Live orders: 1
Reports: 9
```

### `llx_bench`

```bash
./build-llx/llx_bench
```

Observed baseline from the latest run:

- `warmup_orders = 20000`
- `measured_orders = 200000`
- `reports_seen = 412500`
- `fills_seen = 192500`
- `throughput_msgs_per_sec ~= 3.52e+06`

The benchmark binary also prints prototype latency summaries, but those are intentionally not treated as stable README claims yet. At this stage they are more useful for local regression tracking than for headline performance numbers.

## Determinism And Replay

Determinism is a first-class design constraint.

- input commands are totally ordered
- one matching thread processes them in sequence
- there is no concurrent mutation of book state
- the same replay stream should produce the same matching behavior

This matters because reproducibility is what makes optimization work credible. If a benchmark run is not stable enough to compare before/after behavior, it is much less useful.

## Benchmarking Philosophy

The benchmark harness is intentionally modest and explicit.

What it tries to measure:

- steady-state throughput
- replay-driven engine behavior
- the effect of one queue handoff plus one matching thread
- prototype latency distribution

What it does not claim yet:

- production-grade latency numbers
- scheduler-noise-free measurements
- calibrated cycle-accurate timing
- exchange-like tail guarantees

Current measurement caveats:

- wall-clock timestamps are still used
- Linux scheduling noise still exists
- the harness is useful for regression tracking more than final performance claims
- percentile results should be treated as prototype-stage until timing methodology is tightened further

Near-term rigor improvements:

- stronger warmup discipline
- isolated core runs
- release/LTO-only benchmark reporting
- TSC-based timing experiments
- repeatability checks across multiple runs
- clearer coordinated-omission discussion

## Current Implementation Scope

The project is intentionally limited to a believable student-sized target:

- one symbol
- one bounded price range
- one SPSC queue
- one matching thread
- replay-generated traffic
- in-memory only

That scope is a feature, not a limitation. It makes the system easier to understand deeply and optimize honestly.

## Minimal High-Signal Feature Set

The feature set I want this project to stop at, unless each addition is well justified:

- deterministic add / cancel / modify / match
- bounded lock-free SPSC queue
- fixed-size object pool
- ladder-based order book
- direct order-id lookup
- replayable benchmark harness
- correctness tests
- careful benchmark notes

Anything beyond that should earn its complexity.

## Performance Goals

The project should be ambitious but believable.

Good goals for this prototype:

- zero allocator activity in the hot path after startup
- stable correctness under replay
- measurable throughput improvements from structural changes
- clear before/after benchmark evidence for optimizations
- latency methodology that becomes more rigorous over time

The most valuable outcome is not a huge number in the README. It is a small codebase whose performance behavior you can explain clearly.

## Why This Is Still Impressive

For strong systems or HFT interviews, this kind of project is compelling if it shows:

- disciplined scope control
- strong ownership of implementation details
- careful data-structure choices
- awareness of cache and scheduling effects
- honest benchmarking methodology
- reproducible experiments

A small system that is genuinely understood is usually more impressive than a large system that looks half-implemented or over-abstracted.

## Current Status

The repository already contains the first simplified version of this direction:

- `std::map`-style prototype code has been replaced by a ladder-style book
- the queueing story has been reduced to one SPSC queue
- the benchmark path has been simplified to producer -> queue -> matching thread
- the project has been stripped of cloud / deployment / observability framing

Still worth improving:

- simplify some interfaces further
- tighten benchmark methodology
- document benchmark hardware and run conditions
- reduce incidental complexity in the hot path where justified
