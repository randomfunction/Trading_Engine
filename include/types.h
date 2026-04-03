#pragma once
#include <cstdint>

// Target: STL containers and simple types
using OrderId = uint64_t;
using Price = uint64_t;
using Quantity = uint32_t;

// Clean order structure
struct Order {
    OrderId id;
    Price price;
    Quantity quantity;
    bool is_buy;        // side: true for buy, false for sell
    uint64_t timestamp; // precise time ordering if needed
};
