# Small Low-Latency Matching Engine

A compact C++17 matching engine prototype built for low-latency systems learning and interview discussion.  
It focuses on a small set of high-signal ideas: single-writer ownership, lock-free SPSC handoff, bounded memory, simple data layouts, and replayable benchmarking.  
The goal is depth, not scope: one symbol, one process, one matching thread, and a codebase that can be understood end-to-end.

## Overview

This project is a deliberately small low-latency engine prototype.

It is designed to be:

- understandable by one developer
- realistic for a strong student / new-grad systems project
- technically credible for HFT and performance-focused backend interviews
- implementation-focused rather than architecture-heavy

It is not trying to look like a production exchange or a distributed trading platform.

## What This Project Demonstrates

- Deterministic price-time-priority matching
- Single-writer ownership of the order book
- Lock-free single-producer / single-consumer queueing
- Fixed-size object-pool allocation for order storage
- Ladder-based order book layout over a bounded price range
- Reproducible replay and benchmark runs
- Careful, caveated performance measurement

## Architecture

The system is intentionally minimal:

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

- One producer thread feeds commands into the queue.
- One consumer thread owns and mutates all book state.
- The queue is bounded and preallocated.
- The order book uses bounded in-memory storage.
- Input is replayed in a totally ordered sequence.
- Queue synchronization uses acquire / release atomics only.

This keeps the concurrency model simple while preserving the core low-latency ideas that actually matter.

## Core Ideas

### `SPSC ring buffer`

The hot-path handoff uses a bounded single-producer / single-consumer ring:

- no mutexes in the handoff path
- fixed capacity
- cache-line-separated producer / consumer state
- acquire / release synchronization only

### `Single-writer matching core`

Only one thread mutates the book:

- simpler correctness story
- no shared-state locking in the hot path
- deterministic command processing

### `Ladder-based order book`

The book is designed around a bounded price range:

- direct price-to-index mapping
- FIFO behavior at each price level
- direct order-id lookup for cancel / modify
- index-linked intrusive lists instead of heap pointers
- less pointer-heavy access than tree-based containers

### `Fixed-size object pool`

Orders are stored in a bounded pool:

- no steady-state heap allocation in the hot path
- explicit capacity limits
- easier lifetime reasoning

## Design Tradeoffs

This project chooses a few narrow designs on purpose.

- `SPSC` instead of `MPMC`: one producer and one consumer are enough for this prototype, and the implementation is smaller, easier to verify, and cheaper to synchronize.
- Single-writer matching: one thread owns the book so correctness is easier to reason about and the hot path avoids lock contention entirely.
- One symbol: this keeps the matching logic, price ladder, and replay behavior easy to understand without introducing routing or sharding complexity.
- Bounded price ladder instead of a general-purpose tree: the code assumes a known price range and trades flexibility for direct indexing and simpler traversal.
- Bounded memory and object pools: capacity is explicit, allocator noise is removed from steady-state processing, and failure modes are easier to reason about.

These are not universal choices. They are choices that fit a small, measurable systems project.

## Mechanical Sympathy

This project intentionally keeps only a few high-signal low-latency techniques:

- cache-line padding on hot queue state
- producer / consumer index separation to reduce false sharing
- contiguous arrays for price levels and pooled objects
- bounded memory usage
- reduced pointer chasing
- allocator avoidance after initialization
- deterministic event ordering
- simple branch structure in common paths

The goal is not to include every advanced optimization. The goal is to implement a few important ones well enough to explain and measure honestly.

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
│   ├── bench/
│   ├── book/
│   ├── core/
│   ├── memory/
│   ├── os/
│   ├── queue/
│   └── time/
└── src/
    ├── book/
    └── core/
```

## Build

```bash
cmake -S . -B build-llx
cmake --build build-llx -j
```

## Run

### Tests

```bash
./build-llx/llx_tests
```

### Deterministic replay

```bash
./build-llx/llx_replay
```

### Benchmark

```bash
./build-llx/llx_bench
```

## Latest Verified Outputs

Verified locally on May 11, 2026 from the current `build-llx` tree.

### `llx_tests`

```text
llx_tests: passed
```

### `llx_replay`

```text
Replay complete
Trades: 2
Live orders: 1
Reports: 9
```

### `llx_bench`

Latest baseline run:

- `warmup_orders = 20000`
- `measured_orders = 200000`
- `reports_seen = 412500`
- `fills_seen = 192500`
- `throughput_msgs_per_sec ~= 3.52e+06`

The benchmark binary also prints prototype latency summaries, but those are not treated as stable headline claims yet.

## Benchmark Notes

The benchmark harness is useful today for:

- regression tracking
- replay-driven performance comparison
- validating that structural changes improve or degrade throughput
- release-mode baseline measurements

Current caveats:

- wall-clock timestamps are still used
- scheduler noise still exists
- benchmark methodology is still being tightened
- latency percentiles are prototype-stage measurements, not polished claims
- fixed-capacity queue behavior can influence results under sustained backpressure

Planned improvements:

- stronger warmup discipline
- isolated-core benchmark runs
- release/LTO-only reporting
- TSC-based timing experiments
- repeatability checks across runs

## Why This Project Is Credible

This project is intentionally small enough to understand fully, but still interesting enough to discuss in a serious systems interview.

The value is not “big architecture.” The value is:

- careful scope control
- good data-structure choices
- honest measurement
- clear ownership and concurrency boundaries
- a codebase that can be defended line-by-line

## Current Scope

The project intentionally stops at:

- one symbol
- one process
- one queue
- one matching thread
- one bounded in-memory book
- replay-generated traffic

That is a feature, not a limitation. It keeps the system understandable and the engineering claims believable.
