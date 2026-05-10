#pragma once

#include <cstddef>

#include "llx/book/ladder_book.hpp"
#include "llx/core/types.hpp"

namespace llx::core {

class MatchingEngine {
public:
    explicit MatchingEngine(const llx::book::LadderBook::Config& config);

    bool on_command(const OrderCommand& command, const llx::book::LadderBook::ReportHandler& report_handler);
    [[nodiscard]] std::size_t drain_reject_free(const OrderCommand* commands,
                                                std::size_t count,
                                                const llx::book::LadderBook::ReportHandler& report_handler);

    [[nodiscard]] const llx::book::LadderBook& book() const noexcept { return book_; }

private:
    llx::book::LadderBook book_;
};

}  // namespace llx::core
