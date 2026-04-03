#pragma once
#include "types.h"
#include "fast_bitmap.h"
#include "metrics.h"
#include <vector>
#include <unordered_map>
#include <chrono>

struct LevelData {
    Order* head = nullptr;
    Order* tail = nullptr;
};

class OrderBook {
    std::vector<LevelData> book;
    std::unordered_map<OrderId, Order*> order_map;
    std::vector<Order> order_pool;
    Order* free_list_head = nullptr;
    FastBitmap buy_bitmap;
    FastBitmap sell_bitmap;
    uint32_t trades_executed = 0;

    Order* allocateOrder();
    void freeOrder(Order* order);
    void removeOrderFromList(Order* order);

public:
    OrderBook();
    bool addOrder(OrderId id, Price price, Quantity quantity, bool is_buy);
    void cancelOrder(OrderId id);
    void match();
    uint32_t getTrades() const { return trades_executed; }
};
