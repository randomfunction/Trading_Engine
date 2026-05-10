#pragma once

#include <functional>
#include <vector>

#include "llx/core/types.hpp"
#include "llx/memory/object_pool.hpp"

namespace llx::book {

class LadderBook {
public:
    struct Config {
        llx::core::Price min_price{0};
        llx::core::Price max_price{200000};
        std::size_t max_orders{1U << 20};
        std::size_t max_order_id{1U << 20};
    };

    using ReportHandler = std::function<void(const llx::core::ExecutionReport&)>;

    explicit LadderBook(const Config& config);

    bool add(const llx::core::OrderCommand& cmd, const ReportHandler& report_handler);
    bool cancel(const llx::core::OrderCommand& cmd, const ReportHandler& report_handler);
    bool modify(const llx::core::OrderCommand& cmd, const ReportHandler& report_handler);

    [[nodiscard]] std::uint64_t trades() const noexcept { return trades_; }
    [[nodiscard]] std::uint64_t live_orders() const noexcept { return live_orders_; }
    [[nodiscard]] llx::core::Price best_bid() const noexcept;
    [[nodiscard]] llx::core::Price best_ask() const noexcept;

private:
    struct OrderNode {
        llx::core::OrderId order_id{0};
        llx::core::Price price{0};
        llx::core::Quantity quantity{0};
        llx::core::Side side{llx::core::Side::buy};
        std::uint32_t prev{llx::core::invalid_index};
        std::uint32_t next{llx::core::invalid_index};
        bool active{false};
    };

    struct PriceLevel {
        std::uint32_t head{llx::core::invalid_index};
        std::uint32_t tail{llx::core::invalid_index};
        llx::core::Quantity total_qty{0};
        std::uint32_t order_count{0};
    };

    [[nodiscard]] std::uint32_t price_to_index(llx::core::Price price) const noexcept;
    [[nodiscard]] bool is_price_valid(llx::core::Price price) const noexcept;
    void enqueue(std::uint32_t level_idx, std::uint32_t order_idx);
    void unlink(std::uint32_t level_idx, std::uint32_t order_idx);
    void flush_inactive_front(std::uint32_t level_idx);
    void match(const ReportHandler& report_handler);
    void update_best_bid() noexcept;
    void update_best_ask() noexcept;

    Config config_;
    memory::ObjectPool<OrderNode> pool_;
    std::vector<PriceLevel> bids_;
    std::vector<PriceLevel> asks_;
    std::vector<std::uint32_t> order_lookup_;
    std::int32_t best_bid_idx_{-1};
    std::int32_t best_ask_idx_{-1};
    std::uint64_t trades_{0};
    std::uint64_t live_orders_{0};
};

}  // namespace llx::book
