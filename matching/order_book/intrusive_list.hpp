#pragma once

#include <domain/order_view.hpp>

namespace fluxtrade {

class IntrusiveOrderList {
public:
    IntrusiveOrderList() = default;
    ~IntrusiveOrderList() = default;

    IntrusiveOrderList(const IntrusiveOrderList&) = delete;
    IntrusiveOrderList& operator=(const IntrusiveOrderList&) = delete;

    [[nodiscard]] LiveOrder* head() const noexcept { return head_; }
    [[nodiscard]] LiveOrder* tail() const noexcept { return tail_; }
    [[nodiscard]] bool empty() const noexcept { return head_ == nullptr; }

    // Appends an order to the tail of the list in O(1)
    void append(LiveOrder* order) noexcept {
        if (!order) return;
        order->next = nullptr;
        order->prev = tail_;
        if (tail_) {
            tail_->next = order;
        } else {
            head_ = order;
        }
        tail_ = order;
    }

    // Removes an order from anywhere in the list in O(1)
    void remove(LiveOrder* order) noexcept {
        if (!order) return;
        if (order->prev) {
            order->prev->next = order->next;
        } else {
            head_ = order->next;
        }
        if (order->next) {
            order->next->prev = order->prev;
        } else {
            tail_ = order->prev;
        }
        order->next = nullptr;
        order->prev = nullptr;
    }

private:
    LiveOrder* head_ = nullptr;
    LiveOrder* tail_ = nullptr;
};

} // namespace fluxtrade
