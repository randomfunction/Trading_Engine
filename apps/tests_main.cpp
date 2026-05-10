#include <cassert>
#include <iostream>
#include <vector>

#include "llx/core/matching_engine.hpp"

int main() {
    llx::book::LadderBook::Config config;
    config.min_price = 90000;
    config.max_price = 110000;
    config.max_orders = 1024;
    config.max_order_id = 4096;

    llx::core::MatchingEngine engine(config);
    std::vector<llx::core::ExecutionReport> reports;
    auto collect = [&](const llx::core::ExecutionReport& report) {
        reports.push_back(report);
    };

    engine.on_command({llx::core::CommandType::add, 1, 100000, 10, llx::core::Side::buy, 1}, collect);
    engine.on_command({llx::core::CommandType::add, 2, 100010, 5, llx::core::Side::sell, 2}, collect);
    assert(engine.book().trades() == 0);

    engine.on_command({llx::core::CommandType::add, 3, 100000, 3, llx::core::Side::sell, 3}, collect);
    assert(engine.book().trades() == 1);

    engine.on_command({llx::core::CommandType::cancel, 1, 0, 0, llx::core::Side::buy, 4}, collect);
    assert(engine.book().live_orders() == 1);

    engine.on_command({llx::core::CommandType::add, 4, 99990, 2, llx::core::Side::buy, 5}, collect);
    engine.on_command({llx::core::CommandType::modify, 4, 100010, 2, llx::core::Side::buy, 6}, collect);
    assert(engine.book().trades() == 2);

    std::cout << "llx_tests: passed\n";
    return 0;
}
