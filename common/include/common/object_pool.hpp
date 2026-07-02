#pragma once

#include <vector>
#include <new>
#include <cstddef>
#include <utility>
#include <stdexcept>
#include <type_traits>

namespace fluxtrade {

template <typename T, size_t Capacity>
class ObjectPool {
public:
    ObjectPool() {
        free_list_.reserve(Capacity);
        for (size_t i = 0; i < Capacity; ++i) {
            free_list_.push_back(i);
        }
    }

    ~ObjectPool() {
        // Destroy any active objects
        // In a strictly performance-focused arena, we assume objects are returned.
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    template <typename... Args>
    T* allocate(Args&&... args) {
        if (free_list_.empty()) {
            throw std::bad_alloc();
        }

        const size_t index = free_list_.back();
        free_list_.pop_back();

        T* ptr = reinterpret_cast<T*>(&storage_[index]);
        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }

    void deallocate(T* ptr) noexcept {
        if (!ptr) return;

        // Calculate index
        const auto* storage_start = reinterpret_cast<const StorageType*>(&storage_[0]);
        const auto* storage_ptr = reinterpret_cast<const StorageType*>(ptr);
        const ptrdiff_t index = storage_ptr - storage_start;

        if (index < 0 || index >= static_cast<ptrdiff_t>(Capacity)) {
            // Out of bounds pointer deallocation
            return;
        }

        // Call destructor
        ptr->~T();

        free_list_.push_back(static_cast<size_t>(index));
    }

    [[nodiscard]] size_t available() const noexcept {
        return free_list_.size();
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return Capacity;
    }

private:
    using StorageType = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

    StorageType storage_[Capacity];
    std::vector<size_t> free_list_;
};

} // namespace fluxtrade
