#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "llx/core/matching_engine.hpp"
#include "llx/metrics/latency_histogram.hpp"
#include "llx/queue/spsc_ring.hpp"
#include "llx/util/affinity.hpp"
#include "llx/util/time.hpp"

namespace {

constexpr std::size_t ring_capacity = 1U << 16;
constexpr std::uint32_t total_orders = 200000;
constexpr std::uint32_t warmup_orders = 20000;

struct BenchState {
    std::vector<std::uint64_t> latencies_ns;
    std::uint64_t reports_seen{0};
    std::uint64_t accepted_seen{0};
    std::uint64_t trades_seen{0};
    std::uint64_t start_ns{0};
    std::uint64_t end_ns{0};
};

}  // namespace

int main() {
    llx::book::LadderBook::Config config;
    config.min_price = 99500;
    config.max_price = 100500;
    config.max_orders = total_orders + warmup_orders + 1024;
    config.max_order_id = total_orders + warmup_orders + 1024;

    llx::queue::SpscRing<llx::core::OrderCommand, ring_capacity> ingress;
    std::atomic<bool> producer_done{false};
    BenchState state;
    state.latencies_ns.resize(total_orders, 0);

    std::thread matcher([&] {
        (void)llx::util::pin_this_thread(1);
        llx::core::MatchingEngine engine(config);
        llx::core::OrderCommand cmd;

        while (!producer_done.load(std::memory_order_acquire) || ingress.size_approx() > 0) {
            while (ingress.try_pop(cmd)) {
                engine.on_command(cmd, [&](const llx::core::ExecutionReport& report) {
                    ++state.reports_seen;
                    if (report.type == llx::core::ReportType::filled) {
                        ++state.trades_seen;
                    }

                    if (report.type != llx::core::ReportType::accepted) {
                        return;
                    }

                    if (report.order_id <= warmup_orders) {
                        return;
                    }

                    if (state.accepted_seen == 0) {
                        state.start_ns = llx::util::now_ns();
                    }

                    const std::uint32_t sample_index = report.order_id - warmup_orders - 1;
                    state.latencies_ns[sample_index] = llx::util::now_ns() - report.submit_ts_ns;
                    ++state.accepted_seen;
                    if (state.accepted_seen == total_orders) {
                        state.end_ns = llx::util::now_ns();
                    }
                });
            }
        }
    });

    std::thread producer([&] {
        (void)llx::util::pin_this_thread(0);
        const std::uint32_t total_messages = warmup_orders + total_orders;
        for (std::uint32_t id = 1; id <= total_messages; ++id) {
            llx::core::OrderCommand cmd;
            cmd.type = llx::core::CommandType::add;
            cmd.order_id = id;
            cmd.price = 100000 + static_cast<llx::core::Price>(id % 32) - 16;
            cmd.quantity = 10;
            cmd.side = (id % 4U) < 2U ? llx::core::Side::buy : llx::core::Side::sell;
            cmd.submit_ts_ns = llx::util::now_ns();
            while (!ingress.try_push(cmd)) {
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    producer.join();
    matcher.join();

    llx::metrics::LatencyHistogram histogram(total_orders);
    for (const std::uint64_t latency : state.latencies_ns) {
        histogram.record(latency);
    }
    const auto summary = histogram.summarize();

    const std::uint64_t elapsed_ns = state.end_ns - state.start_ns;
    const double seconds = static_cast<double>(elapsed_ns) / 1e9;
    const double throughput = static_cast<double>(total_orders) / seconds;

    std::cout << "llx_bench\n";
    std::cout << "warmup_orders: " << warmup_orders << "\n";
    std::cout << "measured_orders: " << total_orders << "\n";
    std::cout << "reports_seen: " << state.reports_seen << "\n";
    std::cout << "fills_seen: " << state.trades_seen << "\n";
    std::cout << "throughput_msgs_per_sec: " << throughput << "\n";
    std::cout << "latency_min_ns: " << summary.min_ns << "\n";
    std::cout << "latency_p50_ns: " << summary.p50_ns << "\n";
    std::cout << "latency_p99_ns: " << summary.p99_ns << "\n";
    std::cout << "latency_p999_ns: " << summary.p999_ns << "\n";
    std::cout << "latency_max_ns: " << summary.max_ns << "\n";
    std::cout << "latency_avg_ns: " << summary.average_ns << "\n";
    return 0;
}
