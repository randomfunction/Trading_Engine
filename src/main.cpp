#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <random>
#include <iomanip>
#include "order_book.h"

#include <prometheus/exposer.h>
#include "order_book.h"

// [Tests and Benchmarks omitted for brevity, keeping original logic]
void runTests() {
    OrderBook book;
    bool ok = book.addOrder(999, 1000000, 10, true);
    assert(!ok);
    book.addOrder(1, 10050, 10, true);   
    book.addOrder(2, 10000, 5, false);   
    assert(book.getTrades() == 1);
    book.addOrder(3, 10020, 10, true);
    book.cancelOrder(3); 
    book.addOrder(4, 10020, 10, false); 
    assert(book.getTrades() == 2); 
    book.addOrder(5, 10100, 20, true); 
    assert(book.getTrades() == 3);
    book.addOrder(6, 10090, 10, false);
    assert(book.getTrades() == 4);
    book.addOrder(7, 10095, 10, false);
    assert(book.getTrades() == 5);
    book.addOrder(8, 10200, 10, false);
    book.addOrder(9, 10210, 10, false);
    book.addOrder(10, 10220, 25, true); 
    assert(book.getTrades() == 8);
    std::cout << "Unit Tests Passed.\n";
}

struct TestOrder {
    OrderId id; Price price; Quantity quantity; bool is_buy;
};

void runBenchmarks() {
    constexpr size_t NUM_ORDERS = 1000000;
    std::vector<TestOrder> orders(NUM_ORDERS);
    std::mt19937 gen(42); 
    std::uniform_int_distribution<Price> price_dist(90000, 110000); 
    std::uniform_int_distribution<Quantity> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    for (size_t i = 0; i < NUM_ORDERS; ++i) {
        orders[i] = {static_cast<OrderId>(i + 1), price_dist(gen), qty_dist(gen), side_dist(gen) == 1};
    }
    OrderBook book;
    std::cout << "Starting benchmark...\n";
    auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < NUM_ORDERS; ++i) {
        book.addOrder(orders[i].id, orders[i].price, orders[i].quantity, orders[i].is_buy);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end_time - start_time;
    std::cout << "Throughput: " << NUM_ORDERS / diff.count() << " orders/sec\n";
}

int main(int argc, char* argv[]) {
    prometheus::Exposer exposer{"0.0.0.0:8080"};
    exposer.RegisterCollectable(Metrics::instance().registry);

    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--test") { runTests(); return 0; }
        else if (arg == "--benchmark") { runBenchmarks(); return 0; }
        else if (arg == "--all") { runTests(); runBenchmarks(); return 0; }
    }
    
    std::cout << "Trading Engine Active at :8080. Simulating orders...\n";
    OrderBook book;
    OrderId id = 0;
    while(true) {
        book.addOrder(++id, 100000 + (rand() % 1000), 10, (rand() % 2 == 0));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}
