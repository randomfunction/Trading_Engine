#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace llx::metrics {

struct LatencySummary {
    std::uint64_t min_ns{0};
    std::uint64_t p50_ns{0};
    std::uint64_t p99_ns{0};
    std::uint64_t p999_ns{0};
    std::uint64_t max_ns{0};
    double average_ns{0.0};
};

class LatencyHistogram {
public:
    explicit LatencyHistogram(std::size_t reserve_count = 0) {
        samples_.reserve(reserve_count);
    }

    void record(std::uint64_t ns) {
        samples_.push_back(ns);
        total_ns_ += static_cast<long double>(ns);
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return samples_.size();
    }

    [[nodiscard]] LatencySummary summarize() {
        if (samples_.empty()) {
            throw std::runtime_error("No latency samples recorded");
        }

        std::sort(samples_.begin(), samples_.end());
        LatencySummary out;
        out.min_ns = samples_.front();
        out.max_ns = samples_.back();
        out.p50_ns = percentile(0.50);
        out.p99_ns = percentile(0.99);
        out.p999_ns = percentile(0.999);
        out.average_ns = static_cast<double>(total_ns_ / samples_.size());
        return out;
    }

private:
    std::uint64_t percentile(double p) const {
        const double idx = p * static_cast<double>(samples_.size() - 1);
        return samples_[static_cast<std::size_t>(idx)];
    }

    std::vector<std::uint64_t> samples_;
    long double total_ns_{0.0};
};

}  // namespace llx::metrics
