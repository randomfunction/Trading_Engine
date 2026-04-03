#include "order_book.h"
#include <algorithm>
#include <chrono>
#include <iostream>

bool OrderBook::add_order(OrderId id, Price price, Quantity quantity, bool is_buy) {
    Metrics::instance().orders_received.Increment();
    auto start = std::chrono::high_resolution_clock::now();

    // Create the order with an incrementing timestamp
    Order order = {id, price, quantity, is_buy, ++current_timestamp};

    // Mark as active in the system
    active_orders[id] = true;
    Metrics::instance().order_book_depth.Increment();

    // Place the order in the appropriate queue
    if (is_buy) {
        bids[price].push(order);
    } else {
        asks[price].push(order);
    }

    // After adding, always try to match the order book
    match_order();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    Metrics::instance().match_latency.Observe(elapsed.count());
    
    return true;
}

void OrderBook::cancel_order(OrderId id) {
    // Lazy cancellation: we just mark the order as inactive by ID.
    // The physical order will be thrown away when it reaches the front of its queue.
    auto it = active_orders.find(id);
    if (it != active_orders.end() && it->second) {
        it->second = false; // Mark inactive
        Metrics::instance().order_book_depth.Decrement();
    }
}

void OrderBook::match_order() {
    uint32_t initial_trades = trades_executed;

    // Keep matching as long as there are both bids and asks
    while (!bids.empty() && !asks.empty()) {
        auto best_bid_it = bids.begin();
        auto best_ask_it = asks.begin();

        Price best_bid_price = best_bid_it->first;
        Price best_ask_price = best_ask_it->first;

        // Highest bid is lower than lowest ask -> no crossed market, stop matching
        if (best_bid_price < best_ask_price) {
            break;
        }

        auto& bid_queue = best_bid_it->second;
        auto& ask_queue = best_ask_it->second;

        // Clean out any cancelled orders at the front of the best bid queue
        while (!bid_queue.empty() && !active_orders[bid_queue.front().id]) {
            bid_queue.pop();
        }
        
        // Clean out any cancelled orders at the front of the best ask queue
        while (!ask_queue.empty() && !active_orders[ask_queue.front().id]) {
            ask_queue.pop();
        }

        // Check if either queue became empty, and remove the level if so
        if (bid_queue.empty()) {
            bids.erase(best_bid_it);
            continue; // Re-evaluate market state
        }
        if (ask_queue.empty()) {
            asks.erase(best_ask_it);
            continue; // Re-evaluate market state
        }

        // Both top orders are active and prices cross -> Execute a Trade!
        Order& top_bid = bid_queue.front();
        Order& top_ask = ask_queue.front();

        // The traded quantity is the minimum of both available quantities
        Quantity traded_qty = std::min(top_bid.quantity, top_ask.quantity);
        
        // Deduct traded quantity (in-place modification via reference)
        top_bid.quantity -= traded_qty;
        top_ask.quantity -= traded_qty;
        trades_executed++;

        // If bid order is fully filled, remove it
        if (top_bid.quantity == 0) {
            active_orders[top_bid.id] = false;
            Metrics::instance().order_book_depth.Decrement();
            bid_queue.pop();
            if (bid_queue.empty()) {
                bids.erase(best_bid_it);
            }
        }
        
        // If ask order is fully filled, remove it
        if (top_ask.quantity == 0) {
            active_orders[top_ask.id] = false;
            Metrics::instance().order_book_depth.Decrement();
            ask_queue.pop();
            if (ask_queue.empty()) {
                asks.erase(best_ask_it);
            }
        }
    }

    if (trades_executed > initial_trades) {
        Metrics::instance().trades_executed.Increment(trades_executed - initial_trades);
    }
}

void OrderBook::print_order_book() const {
    std::cout << "=== ORDER BOOK ===\n";
    
    std::cout << "--- ASKS ---\n";
    // Asks map is ascending. We print reverse so the highest ask is printed first (top of visually appealing book)
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        Price price = it->first;
        std::queue<Order> temp_queue = it->second; 
        Quantity total_qty = 0;
        
        while (!temp_queue.empty()) {
            const Order& order = temp_queue.front();
            if (active_orders.at(order.id)) {
                total_qty += order.quantity;
            }
            temp_queue.pop();
        }
        
        if (total_qty > 0) {
            std::cout << "Price: " << price << " | Qty: " << total_qty << "\n";
        }
    }
    
    std::cout << "------------\n";
    std::cout << "--- BIDS ---\n";
    
    // Bids map is descending. Normal iteration prints highest bids first.
    for (const auto& [price, queue_orig] : bids) {
        std::queue<Order> temp_queue = queue_orig; 
        Quantity total_qty = 0;
        
        while (!temp_queue.empty()) {
            const Order& order = temp_queue.front();
            if (active_orders.at(order.id)) {
                total_qty += order.quantity;
            }
            temp_queue.pop();
        }
        
        if (total_qty > 0) {
            std::cout << "Price: " << price << " | Qty: " << total_qty << "\n";
        }
    }
    
    std::cout << "==================\n";
}
