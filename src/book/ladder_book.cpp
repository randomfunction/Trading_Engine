#include "llx/book/ladder_book.hpp"

#include <algorithm>

namespace llx::book {

using llx::core::CommandType;
using llx::core::ExecutionReport;
using llx::core::OrderCommand;
using llx::core::OrderId;
using llx::core::Price;
using llx::core::Quantity;
using llx::core::ReportType;
using llx::core::Side;
using llx::core::invalid_index;

namespace {

ExecutionReport make_report(ReportType type,
                            OrderId order_id,
                            OrderId match_order_id,
                            Price price,
                            Quantity quantity,
                            std::uint64_t submit_ts_ns) {
    ExecutionReport report;
    report.type = type;
    report.order_id = order_id;
    report.match_order_id = match_order_id;
    report.price = price;
    report.quantity = quantity;
    report.submit_ts_ns = submit_ts_ns;
    report.complete_ts_ns = submit_ts_ns;
    return report;
}

}  // namespace

LadderBook::LadderBook(const Config& config)
    : config_(config),
      pool_(config.max_orders),
      bids_(static_cast<std::size_t>(config.max_price - config.min_price + 1)),
      asks_(static_cast<std::size_t>(config.max_price - config.min_price + 1)),
      order_lookup_(config.max_order_id + 1, invalid_index) {}

std::uint32_t LadderBook::price_to_index(Price price) const noexcept {
    return static_cast<std::uint32_t>(price - config_.min_price);
}

bool LadderBook::is_price_valid(Price price) const noexcept {
    return price >= config_.min_price && price <= config_.max_price;
}

Price LadderBook::best_bid() const noexcept {
    if (best_bid_idx_ < 0) {
        return config_.min_price - 1;
    }
    return static_cast<Price>(config_.min_price + best_bid_idx_);
}

Price LadderBook::best_ask() const noexcept {
    if (best_ask_idx_ < 0) {
        return config_.max_price + 1;
    }
    return static_cast<Price>(config_.min_price + best_ask_idx_);
}

void LadderBook::enqueue(std::uint32_t level_idx, std::uint32_t order_idx) {
    auto& level = pool_.at(order_idx).side == Side::buy ? bids_[level_idx] : asks_[level_idx];
    auto& node = pool_.at(order_idx);
    node.prev = level.tail;
    node.next = invalid_index;

    if (level.tail == invalid_index) {
        level.head = order_idx;
    } else {
        pool_.at(level.tail).next = order_idx;
    }

    level.tail = order_idx;
    level.total_qty += node.quantity;
    ++level.order_count;
}

void LadderBook::unlink(std::uint32_t level_idx, std::uint32_t order_idx) {
    auto& node = pool_.at(order_idx);
    auto& level = node.side == Side::buy ? bids_[level_idx] : asks_[level_idx];

    if (node.prev != invalid_index) {
        pool_.at(node.prev).next = node.next;
    } else {
        level.head = node.next;
    }

    if (node.next != invalid_index) {
        pool_.at(node.next).prev = node.prev;
    } else {
        level.tail = node.prev;
    }

    level.total_qty -= node.quantity;
    --level.order_count;
    node.prev = invalid_index;
    node.next = invalid_index;
}

void LadderBook::flush_inactive_front(std::uint32_t level_idx) {
    auto& bid_level = bids_[level_idx];
    while (bid_level.head != invalid_index) {
        const std::uint32_t dead_idx = bid_level.head;
        auto& head = pool_.at(dead_idx);
        if (head.active && head.quantity > 0) {
            break;
        }
        unlink(level_idx, dead_idx);
        pool_.release(dead_idx);
    }

    auto& ask_level = asks_[level_idx];
    while (ask_level.head != invalid_index) {
        const std::uint32_t dead_idx = ask_level.head;
        auto& head = pool_.at(dead_idx);
        if (head.active && head.quantity > 0) {
            break;
        }
        unlink(level_idx, dead_idx);
        pool_.release(dead_idx);
    }
}

void LadderBook::update_best_bid() noexcept {
    for (std::int32_t i = static_cast<std::int32_t>(bids_.size()) - 1; i >= 0; --i) {
        if (bids_[static_cast<std::size_t>(i)].head != invalid_index) {
            best_bid_idx_ = i;
            return;
        }
    }
    best_bid_idx_ = -1;
}

void LadderBook::update_best_ask() noexcept {
    for (std::size_t i = 0; i < asks_.size(); ++i) {
        if (asks_[i].head != invalid_index) {
            best_ask_idx_ = static_cast<std::int32_t>(i);
            return;
        }
    }
    best_ask_idx_ = -1;
}

bool LadderBook::add(const OrderCommand& cmd, const ReportHandler& report_handler) {
    if (!is_price_valid(cmd.price) || cmd.quantity == 0 || cmd.order_id >= order_lookup_.size()) {
        report_handler(make_report(ReportType::rejected, cmd.order_id, 0, cmd.price, cmd.quantity, cmd.submit_ts_ns));
        return false;
    }
    if (order_lookup_[cmd.order_id] != invalid_index) {
        report_handler(make_report(ReportType::rejected, cmd.order_id, 0, cmd.price, cmd.quantity, cmd.submit_ts_ns));
        return false;
    }

    const std::uint32_t order_idx = pool_.allocate();
    auto& node = pool_.at(order_idx);
    node.order_id = cmd.order_id;
    node.price = cmd.price;
    node.quantity = cmd.quantity;
    node.side = cmd.side;
    node.active = true;
    node.prev = invalid_index;
    node.next = invalid_index;

    order_lookup_[cmd.order_id] = order_idx;
    const std::uint32_t level_idx = price_to_index(cmd.price);
    enqueue(level_idx, order_idx);
    ++live_orders_;

    if (cmd.side == Side::buy) {
        best_bid_idx_ = std::max(best_bid_idx_, static_cast<std::int32_t>(level_idx));
    } else if (best_ask_idx_ < 0) {
        best_ask_idx_ = static_cast<std::int32_t>(level_idx);
    } else {
        best_ask_idx_ = std::min(best_ask_idx_, static_cast<std::int32_t>(level_idx));
    }

    report_handler(make_report(ReportType::accepted, cmd.order_id, 0, cmd.price, cmd.quantity, cmd.submit_ts_ns));
    match(report_handler);
    return true;
}

bool LadderBook::cancel(const OrderCommand& cmd, const ReportHandler& report_handler) {
    if (cmd.order_id >= order_lookup_.size()) {
        report_handler(make_report(ReportType::rejected, cmd.order_id, 0, 0, 0, cmd.submit_ts_ns));
        return false;
    }

    const std::uint32_t order_idx = order_lookup_[cmd.order_id];
    if (order_idx == invalid_index) {
        report_handler(make_report(ReportType::rejected, cmd.order_id, 0, 0, 0, cmd.submit_ts_ns));
        return false;
    }

    auto& node = pool_.at(order_idx);
    if (!node.active) {
        report_handler(make_report(ReportType::rejected, cmd.order_id, 0, node.price, 0, cmd.submit_ts_ns));
        return false;
    }

    const std::uint32_t level_idx = price_to_index(node.price);
    const Price cancelled_price = node.price;
    const Side cancelled_side = node.side;
    unlink(level_idx, order_idx);
    node.active = false;
    order_lookup_[cmd.order_id] = invalid_index;
    pool_.release(order_idx);
    --live_orders_;

    if (cancelled_side == Side::buy && best_bid_idx_ == static_cast<std::int32_t>(level_idx) &&
        bids_[level_idx].head == invalid_index) {
        update_best_bid();
    }
    if (cancelled_side == Side::sell && best_ask_idx_ == static_cast<std::int32_t>(level_idx) &&
        asks_[level_idx].head == invalid_index) {
        update_best_ask();
    }

    report_handler(make_report(ReportType::cancelled, cmd.order_id, 0, cancelled_price, 0, cmd.submit_ts_ns));
    return true;
}

bool LadderBook::modify(const OrderCommand& cmd, const ReportHandler& report_handler) {
    OrderCommand cancel_cmd = cmd;
    cancel_cmd.type = CommandType::cancel;
    if (!cancel(cancel_cmd, report_handler)) {
        return false;
    }

    OrderCommand add_cmd = cmd;
    add_cmd.type = CommandType::add;
    return add(add_cmd, report_handler);
}

void LadderBook::match(const ReportHandler& report_handler) {
    while (best_bid_idx_ >= 0 && best_ask_idx_ >= 0 && best_bid() >= best_ask()) {
        auto& bid_level = bids_[static_cast<std::size_t>(best_bid_idx_)];
        auto& ask_level = asks_[static_cast<std::size_t>(best_ask_idx_)];

        if (bid_level.head == invalid_index) {
            update_best_bid();
            continue;
        }
        if (ask_level.head == invalid_index) {
            update_best_ask();
            continue;
        }

        auto& bid = pool_.at(bid_level.head);
        auto& ask = pool_.at(ask_level.head);
        const Quantity traded = std::min(bid.quantity, ask.quantity);
        const Price trade_price = ask.price;

        bid.quantity -= traded;
        ask.quantity -= traded;
        bid_level.total_qty -= traded;
        ask_level.total_qty -= traded;
        ++trades_;

        report_handler(make_report(ReportType::filled, bid.order_id, ask.order_id, trade_price, traded, 0));
        report_handler(make_report(ReportType::filled, ask.order_id, bid.order_id, trade_price, traded, 0));

        if (bid.quantity == 0) {
            const std::uint32_t done_idx = bid_level.head;
            order_lookup_[bid.order_id] = invalid_index;
            unlink(static_cast<std::uint32_t>(best_bid_idx_), done_idx);
            pool_.at(done_idx).active = false;
            pool_.release(done_idx);
            --live_orders_;
            if (bid_level.head == invalid_index) {
                update_best_bid();
            }
        }

        if (ask.quantity == 0) {
            const std::uint32_t done_idx = ask_level.head;
            order_lookup_[ask.order_id] = invalid_index;
            unlink(static_cast<std::uint32_t>(best_ask_idx_), done_idx);
            pool_.at(done_idx).active = false;
            pool_.release(done_idx);
            --live_orders_;
            if (ask_level.head == invalid_index) {
                update_best_ask();
            }
        }
    }
}

}  // namespace llx::book
