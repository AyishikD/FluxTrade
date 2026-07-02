#pragma once

#include <domain/ids.hpp>
#include <domain/order_view.hpp>
#include <unordered_map>

namespace fluxtrade {

class OrderLookup {
public:
    OrderLookup() = default;
    ~OrderLookup() = default;

    OrderLookup(const OrderLookup&) = delete;
    OrderLookup& operator=(const OrderLookup&) = delete;

    [[nodiscard]] LiveOrder* find(const OrderId& order_id) const noexcept {
        auto it = lookup_.find(order_id.get());
        if (it == lookup_.end()) {
            return nullptr;
        }
        return it->second;
    }

    void insert(const OrderId& order_id, LiveOrder* order) noexcept {
        lookup_[order_id.get()] = order;
    }

    void erase(const OrderId& order_id) noexcept {
        lookup_.erase(order_id.get());
    }

    [[nodiscard]] size_t size() const noexcept { return lookup_.size(); }
    
    void clear() noexcept { lookup_.clear(); }

private:
    std::unordered_map<uint64_t, LiveOrder*> lookup_;
};

} // namespace fluxtrade
