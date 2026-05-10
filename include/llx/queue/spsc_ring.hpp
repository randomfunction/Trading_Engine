#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace llx::queue {

template <typename T, std::size_t Capacity>
class SpscRing {
    static_assert(Capacity >= 2, "Capacity must be at least 2");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert(std::is_trivially_copyable<T>::value, "SpscRing requires trivially copyable payloads");

public:
    bool try_push(const T& value) noexcept {
        const std::uint64_t head = head_.load(std::memory_order_relaxed);
        const std::uint64_t next = head + 1;
        if (next - tail_cache_ > Capacity) {
            tail_cache_ = tail_.load(std::memory_order_acquire);
            if (next - tail_cache_ > Capacity) {
                return false;
            }
        }

        storage_[head & mask_] = value;
        head_.store(next, std::memory_order_release);
        return true;
    }

    bool try_pop(T& value) noexcept {
        const std::uint64_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_cache_) {
            head_cache_ = head_.load(std::memory_order_acquire);
            if (tail == head_cache_) {
                return false;
            }
        }

        value = storage_[tail & mask_];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    std::size_t size_approx() const noexcept {
        const std::uint64_t head = head_.load(std::memory_order_acquire);
        const std::uint64_t tail = tail_.load(std::memory_order_acquire);
        return static_cast<std::size_t>(head - tail);
    }

private:
    static constexpr std::uint64_t mask_ = Capacity - 1;

    alignas(64) std::array<T, Capacity> storage_{};
    alignas(64) std::atomic<std::uint64_t> head_{0};
    alignas(64) std::atomic<std::uint64_t> tail_{0};
    alignas(64) std::uint64_t head_cache_{0};
    alignas(64) std::uint64_t tail_cache_{0};
};

}  // namespace llx::queue
