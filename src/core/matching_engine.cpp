#include "llx/core/matching_engine.hpp"

namespace llx::core {

MatchingEngine::MatchingEngine(const llx::book::LadderBook::Config& config)
    : book_(config) {}

bool MatchingEngine::on_command(const OrderCommand& command,
                                const llx::book::LadderBook::ReportHandler& report_handler) {
    switch (command.type) {
        case CommandType::add:
            return book_.add(command, report_handler);
        case CommandType::cancel:
            return book_.cancel(command, report_handler);
        case CommandType::modify:
            return book_.modify(command, report_handler);
    }

    return false;
}

std::size_t MatchingEngine::drain_reject_free(const OrderCommand* commands,
                                              std::size_t count,
                                              const llx::book::LadderBook::ReportHandler& report_handler) {
    std::size_t processed = 0;
    for (; processed < count; ++processed) {
        on_command(commands[processed], report_handler);
    }
    return processed;
}

}  // namespace llx::core
