#pragma once

#include <cstdint>

namespace llx::core {

using OrderId = std::uint32_t;
using Price = std::int32_t;
using Quantity = std::uint32_t;
using Timestamp = std::uint64_t;

enum class Side : std::uint8_t {
    buy = 0,
    sell = 1,
};

enum class CommandType : std::uint8_t {
    add = 0,
    cancel = 1,
    modify = 2,
};

enum class ReportType : std::uint8_t {
    accepted = 0,
    filled = 1,
    cancelled = 2,
    rejected = 3,
};

struct OrderCommand {
    CommandType type{CommandType::add};
    OrderId order_id{0};
    Price price{0};
    Quantity quantity{0};
    Side side{Side::buy};
    Timestamp submit_ts_ns{0};
};

struct ExecutionReport {
    ReportType type{ReportType::accepted};
    OrderId order_id{0};
    OrderId match_order_id{0};
    Price price{0};
    Quantity quantity{0};
    Timestamp submit_ts_ns{0};
    Timestamp complete_ts_ns{0};
};

inline constexpr std::uint32_t invalid_index = UINT32_MAX;

}  // namespace llx::core
