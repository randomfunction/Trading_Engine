#pragma once
#include "types.h"
#include "metrics.h"
#include <map>
#include <queue>
#include <unordered_map>
#include <iostream>

class OrderBook {
private:
    // Bids (buys): sorted in descending order (highest price first)
    std::map<Price, std::queue<Order>, std::greater<Price>> bids;
    
    // Asks (sells): sorted in ascending order (lowest price first)
    std::map<Price, std::queue<Order>> asks;

    // Fast lookup map to track if an order is still active (lazy cancellation)
    std::unordered_map<OrderId, bool> active_orders;

    uint32_t trades_executed = 0;
    uint64_t current_timestamp = 0;

public:
    OrderBook() = default;
    
    // Core requirements
    bool add_order(OrderId id, Price price, Quantity quantity, bool is_buy);
    void match_order();
    void cancel_order(OrderId id);
    void print_order_book() const;
    
    uint32_t get_trades() const { return trades_executed; }
};
