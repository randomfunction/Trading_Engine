#include <iostream>
#include <vector>

#include "llx/core/matching_engine.hpp"
#include "llx/util/time.hpp"

int main() {
    llx::book::LadderBook::Config config;
    config.min_price = 99000;
    config.max_price = 101000;
    config.max_orders = 1U << 16;
    config.max_order_id = 1U << 16;

    llx::core::MatchingEngine engine(config);
    std::vector<llx::core::OrderCommand> replay = {
        {llx::core::CommandType::add, 1, 100000, 10, llx::core::Side::buy, llx::util::now_ns()},
        {llx::core::CommandType::add, 2, 100001, 10, llx::core::Side::buy, llx::util::now_ns()},
        {llx::core::CommandType::add, 3, 100000, 6, llx::core::Side::sell, llx::util::now_ns()},
        {llx::core::CommandType::cancel, 2, 0, 0, llx::core::Side::buy, llx::util::now_ns()},
        {llx::core::CommandType::add, 4, 99999, 3, llx::core::Side::sell, llx::util::now_ns()},
    };

    std::uint64_t reports = 0;
    auto sink = [&](const llx::core::ExecutionReport&) {
        ++reports;
    };

    for (const auto& cmd : replay) {
        engine.on_command(cmd, sink);
    }

    std::cout << "Replay complete\n";
    std::cout << "Trades: " << engine.book().trades() << "\n";
    std::cout << "Live orders: " << engine.book().live_orders() << "\n";
    std::cout << "Reports: " << reports << "\n";
    return 0;
}
