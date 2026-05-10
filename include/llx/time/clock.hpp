#pragma once

#include <chrono>
#include <cstdint>

namespace llx::util {

inline std::uint64_t now_ns() noexcept {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

}  // namespace llx::util
