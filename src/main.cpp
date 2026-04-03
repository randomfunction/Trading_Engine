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

void runTests() {
    OrderBook book;
    
    // The previous tests matched but used different function names
    book.add_order(1, 10050, 10, true);   
    book.add_order(2, 10000, 5, false);   
    assert(book.get_trades() == 1);
    
    book.add_order(3, 10020, 10, true);
    
    book.cancel_order(3); 
    book.add_order(4, 10020, 10, false); 
    assert(book.get_trades() == 2); 
    
    book.add_order(5, 10100, 20, true); 
    assert(book.get_trades() == 3);
    
    book.add_order(6, 10090, 10, false);
    assert(book.get_trades() == 4);
    
    book.add_order(7, 10095, 10, false);
    assert(book.get_trades() == 5);
    
    book.add_order(8, 10200, 10, false);
    book.add_order(9, 10210, 10, false);
    book.add_order(10, 10220, 25, true); 
    assert(book.get_trades() == 8);
    
    std::cout << "Unit Tests Passed.\n";
    book.print_order_book();
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
        book.add_order(orders[i].id, orders[i].price, orders[i].quantity, orders[i].is_buy);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end_time - start_time;
    std::cout << "Throughput: " << NUM_ORDERS / diff.count() << " orders/sec\n";
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--test") { runTests(); return 0; }
        else if (arg == "--benchmark") { runBenchmarks(); return 0; }
        else if (arg == "--all") { runTests(); runBenchmarks(); return 0; }
    }

    prometheus::Exposer exposer{"0.0.0.0:8080"};
    exposer.RegisterCollectable(Metrics::instance().registry);
    
    std::cout << "Trading Engine Active at :8080. Simulating orders...\n";
    OrderBook book;
    OrderId id = 0;
    while(true) {
        book.add_order(++id, 100000 + (rand() % 1000), 10, (rand() % 2 == 0));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}
