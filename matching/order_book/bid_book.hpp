#pragma once

#include <matching/order_book/price_level_index.hpp>
#include <common/object_pool.hpp>
#include <functional>
#include <cassert>

namespace fluxtrade {

class BidBook {
public:
    BidBook() = default;
    ~BidBook() = default;

    BidBook(const BidBook&) = delete;
    BidBook& operator=(const BidBook&) = delete;

    [[nodiscard]] PriceLevelNode* best_level() const noexcept { return best_level_; }
    [[nodiscard]] size_t size() const noexcept { return index_.size(); }
    [[nodiscard]] bool empty() const noexcept { return best_level_ == nullptr; }

    // Finds an existing price level, or returns nullptr
    [[nodiscard]] PriceLevelNode* find_level(const Price& price) const noexcept {
        return index_.find(price);
    }

    // Creates and links a new price level in sorted descending order
    PriceLevelNode* create_level(const Price& price, ObjectPool<PriceLevelNode, 1024>& level_pool) noexcept {
        PriceLevelNode* new_node = level_pool.allocate();
        new_node->price = price;
        new_node->total_qty = Quantity(0);
        new_node->order_count = 0;
        new_node->next = nullptr;
        new_node->prev = nullptr;

        index_.insert(price, new_node);

        // Link into the sorted doubly-linked list
        if (!best_level_) {
            best_level_ = new_node;
        } else if (price > best_level_->price) {
            // New best bid
            new_node->next = best_level_;
            best_level_->prev = new_node;
            best_level_ = new_node;
        } else {
            // Traverse down from best_level_ to find insertion slot
            PriceLevelNode* curr = best_level_;
            while (curr->next && curr->next->price > price) {
                curr = curr->next;
            }
            // Insert after curr
            new_node->next = curr->next;
            new_node->prev = curr;
            if (curr->next) {
                curr->next->prev = new_node;
            }
            curr->next = new_node;
        }
        return new_node;
    }

    // Unlinks and deletes a price level
    void remove_level(PriceLevelNode* node, ObjectPool<PriceLevelNode, 1024>& level_pool) noexcept {
        if (!node) return;
        index_.erase(node->price);

        if (node == best_level_) {
            best_level_ = node->next;
            if (best_level_) {
                best_level_->prev = nullptr;
            }
        } else {
            if (node->prev) {
                node->prev->next = node->next;
            }
            if (node->next) {
                node->next->prev = node->prev;
            }
        }

        level_pool.deallocate(node);
    }

    void clear(ObjectPool<PriceLevelNode, 1024>& level_pool, ObjectPool<LiveOrder, 10240>& order_pool) noexcept {
        PriceLevelNode* curr = best_level_;
        while (curr) {
            PriceLevelNode* next = curr->next;
            // Deallocate all orders belonging to this level
            LiveOrder* order_curr = curr->orders.head();
            while (order_curr) {
                LiveOrder* order_next = order_curr->next;
                order_pool.deallocate(order_curr);
                order_curr = order_next;
            }
            level_pool.deallocate(curr);
            curr = next;
        }
        best_level_ = nullptr;
        index_.clear();
    }

    // Asserts all buy-side invariants
    void verify_invariants() const noexcept {
#ifndef NDEBUG
        if (!best_level_) {
            assert(index_.empty());
            return;
        }
        assert(best_level_->prev == nullptr);
        
        PriceLevelNode* curr = best_level_;
        Price prev_price = curr->price;
        while (curr) {
            assert(curr->price <= prev_price); // Must descend
            assert(curr->has_orders());
            assert(curr->orders.head() != nullptr);
            assert(curr->orders.tail() != nullptr);
            
            // Cross checks
            if (curr->next) {
                assert(curr->next->prev == curr);
            }
            prev_price = curr->price;
            curr = curr->next;
        }
#endif
    }

private:
    PriceLevelNode* best_level_ = nullptr;
    PriceLevelIndex<std::greater<Price>> index_;
};

} // namespace fluxtrade
