#pragma once

#include <matching/order_book/price_level.hpp>
#include <map>

namespace fluxtrade {

// Decoupled PriceLevel index wrapper to isolate sorting tree lookups from Book rules
template <typename Compare>
class PriceLevelIndex {
public:
    PriceLevelIndex() = default;
    ~PriceLevelIndex() = default;

    PriceLevelIndex(const PriceLevelIndex&) = delete;
    PriceLevelIndex& operator=(const PriceLevelIndex&) = delete;

    [[nodiscard]] PriceLevelNode* find(const Price& price) const noexcept {
        auto it = index_.find(price);
        if (it == index_.end()) {
            return nullptr;
        }
        return it->second;
    }

    void insert(const Price& price, PriceLevelNode* node) noexcept {
        index_[price] = node;
    }

    void erase(const Price& price) noexcept {
        index_.erase(price);
    }

    [[nodiscard]] bool empty() const noexcept { return index_.empty(); }
    [[nodiscard]] size_t size() const noexcept { return index_.size(); }
    
    void clear() noexcept { index_.clear(); }

private:
    std::map<Price, PriceLevelNode*, Compare> index_;
};

} // namespace fluxtrade
