#include "order_book.h"
#include <algorithm>
#include <chrono>
#include <iostream>

OrderBook::OrderBook() : buy_bitmap(MAX_PRICE_LEVELS), sell_bitmap(MAX_PRICE_LEVELS) {
    book.resize(MAX_PRICE_LEVELS);
    order_pool.resize(MAX_ORDERS);
    
    for (size_t i = 0; i < MAX_ORDERS - 1; ++i) {
        order_pool[i].next = &order_pool[i + 1];
    }
    order_pool[MAX_ORDERS - 1].next = nullptr;
    free_list_head = &order_pool[0];
}

Order* OrderBook::allocateOrder() {
    if (!free_list_head) return nullptr; 
    Order* node = free_list_head;
    free_list_head = free_list_head->next;
    
    node->prev = nullptr;
    node->next = nullptr;
    return node;
}

void OrderBook::freeOrder(Order* order) {
    if (!order) return;
    Metrics::instance().order_book_depth.Decrement();
    if (order_map.find(order->id) != order_map.end()) {
        order_map.erase(order->id);
    }
    order->next = free_list_head;
    free_list_head = order;
}

void OrderBook::removeOrderFromList(Order* order) {
    Price p = order->price;
    if (order->prev) {
        order->prev->next = order->next;
    } else {
        book[p].head = order->next;
    }

    if (order->next) {
        order->next->prev = order->prev;
    } else {
        book[p].tail = order->prev;
    }

    if (book[p].head == nullptr) {
        if (order->is_buy) {
            buy_bitmap.reset(p);
        } else {
            sell_bitmap.reset(p);
        }
    }
}

bool OrderBook::addOrder(OrderId id, Price price, Quantity quantity, bool is_buy) {
    Metrics::instance().orders_received.Increment();
    auto start = std::chrono::high_resolution_clock::now();

    if (price >= static_cast<Price>(MAX_PRICE_LEVELS)) {
        std::cerr << "Order rejected: Price bounds exceeded.\n";
        return false;
    }

    Order* order = allocateOrder();
    if (!order) return false;

    Metrics::instance().order_book_depth.Increment();
    order->id = id;
    order->price = price;
    order->quantity = quantity;
    order->is_buy = is_buy;

    order_map[id] = order;

    LevelData& level = book[price];
    if (level.tail) {
        level.tail->next = order;
        order->prev = level.tail;
        level.tail = order;
    } else {
        level.head = order;
        level.tail = order;
        if (is_buy) buy_bitmap.set(price);
        else sell_bitmap.set(price);
    }

    match();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    Metrics::instance().match_latency.Observe(elapsed.count());
    return true;
}

void OrderBook::cancelOrder(OrderId id) {
    auto it = order_map.find(id);
    if (it != order_map.end()) {
        Order* order = it->second;
        removeOrderFromList(order);
        freeOrder(order);
    }
}

void OrderBook::match() {
    uint32_t initial_trades = trades_executed;
    while (true) {
        int highest_buy_idx = buy_bitmap.get_highest();
        int lowest_sell_idx = sell_bitmap.get_lowest();

        if (highest_buy_idx == -1 || lowest_sell_idx == -1) break;
        if (highest_buy_idx < lowest_sell_idx) break;

        LevelData& buy_level = book[highest_buy_idx];
        LevelData& sell_level = book[lowest_sell_idx];

        Order* buy_order = buy_level.head;
        Order* sell_order = sell_level.head;

        if (!buy_order || !sell_order) {
            if (!buy_order) buy_bitmap.reset(highest_buy_idx);
            if (!sell_order) sell_bitmap.reset(lowest_sell_idx);
            continue;
        }

        uint32_t traded = std::min(buy_order->quantity, sell_order->quantity);
        buy_order->quantity -= traded;
        sell_order->quantity -= traded;
        trades_executed++;

        if (buy_order->quantity == 0) {
            removeOrderFromList(buy_order);
            freeOrder(buy_order);
        }
        if (sell_order->quantity == 0) {
            removeOrderFromList(sell_order);
            freeOrder(sell_order);
        }
    }
    if (trades_executed > initial_trades) {
        Metrics::instance().trades_executed.Increment(trades_executed - initial_trades);
    }
}
