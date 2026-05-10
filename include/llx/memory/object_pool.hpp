#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace llx::memory {

template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(std::size_t capacity)
        : storage_(capacity), free_list_(capacity), free_top_(capacity) {
        for (std::size_t i = 0; i < capacity; ++i) {
            free_list_[i] = static_cast<std::uint32_t>(capacity - 1 - i);
        }
    }

    std::uint32_t allocate() {
        if (free_top_ == 0) {
            throw std::runtime_error("ObjectPool exhausted");
        }

        --free_top_;
        return free_list_[free_top_];
    }

    void release(std::uint32_t index) noexcept {
        free_list_[free_top_] = index;
        ++free_top_;
    }

    T& at(std::uint32_t index) noexcept {
        return storage_[index];
    }

    const T& at(std::uint32_t index) const noexcept {
        return storage_[index];
    }

    std::size_t capacity() const noexcept {
        return storage_.size();
    }

private:
    std::vector<T> storage_;
    std::vector<std::uint32_t> free_list_;
    std::size_t free_top_;
};

}  // namespace llx::memory
