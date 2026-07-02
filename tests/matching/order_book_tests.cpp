#include <gtest/gtest.h>
#include <matching/order_book/order_book.hpp>
#include <vector>
#include <string>

using namespace fluxtrade;

// Mock OrderBook Listener to record callbacks
class MockOrderBookListener : public IOrderBookListener {
public:
    std::vector<std::string> log;

    void on_order_inserted(const LiveOrder& order) noexcept override {
        log.push_back("insert_" + std::to_string(order.key.order_id.get()));
    }
    void on_order_cancelled(const LiveOrder& order) noexcept override {
        log.push_back("cancel_" + std::to_string(order.key.order_id.get()));
    }
    void on_order_filled(const LiveOrder& order, const Quantity& filled_qty) noexcept override {
        log.push_back("fill_" + std::to_string(order.key.order_id.get()) + "_" + std::to_string(filled_qty.units()));
    }
    void on_price_level_created(const Price& price, Side side) noexcept override {
        log.push_back("level_create_" + std::to_string(price.ticks()) + "_" + (side == Side::Buy ? "buy" : "sell"));
    }
    void on_price_level_removed(const Price& price, Side side) noexcept override {
        log.push_back("level_remove_" + std::to_string(price.ticks()) + "_" + (side == Side::Buy ? "buy" : "sell"));
    }
};

TEST(OrderBookTest, InsertionAndFIFO) {
    MockOrderBookListener listener;
    LimitOrderBook book(&listener);

    Order o1{OrderId(1), Price::from_double(100.50), Quantity::from_double(10), 101, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1), Side::Buy, OrderType::Limit, TimeInForce::GTC};
    Order o2{OrderId(2), Price::from_double(100.50), Quantity::from_double(5), 102, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1), Side::Buy, OrderType::Limit, TimeInForce::GTC};
    Order o3{OrderId(3), Price::from_double(100.40), Quantity::from_double(20), 103, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1), Side::Buy, OrderType::Limit, TimeInForce::GTC};

    book.insert(o1, 1);
    book.insert(o2, 2);
    book.insert(o3, 3);

    book.verify_invariants();

    // Best Bid Level is 100.50
    EXPECT_DOUBLE_EQ(book.best_bid().to_double(), 100.50);

    PriceLevelNode* bid_level = book.best_bid_level();
    ASSERT_NE(bid_level, nullptr);
    EXPECT_EQ(bid_level->order_count, 2);
    EXPECT_DOUBLE_EQ(bid_level->total_qty.to_double(), 15.0);

    // FIFO verification (o1 at head, o2 after)
    LiveOrder* head = bid_level->orders.head();
    ASSERT_NE(head, nullptr);
    EXPECT_EQ(head->key.order_id, OrderId(1));
    EXPECT_EQ(head->next->key.order_id, OrderId(2));

    // Next Level is 100.40
    ASSERT_NE(bid_level->next, nullptr);
    EXPECT_DOUBLE_EQ(bid_level->next->price.to_double(), 100.40);
    EXPECT_EQ(bid_level->next->orders.head()->key.order_id, OrderId(3));

    // Verify Listener Output
    ASSERT_GE(listener.log.size(), 5);
    EXPECT_EQ(listener.log[0], "level_create_10050000000_buy");
    EXPECT_EQ(listener.log[1], "insert_1");
    EXPECT_EQ(listener.log[2], "insert_2");
    EXPECT_EQ(listener.log[3], "level_create_10040000000_buy");
    EXPECT_EQ(listener.log[4], "insert_3");
}

TEST(OrderBookTest, CancellationAndLevelRemoval) {
    MockOrderBookListener listener;
    LimitOrderBook book(&listener);

    Order o1{OrderId(1), Price::from_double(100.50), Quantity::from_double(10), 101, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1), Side::Buy, OrderType::Limit, TimeInForce::GTC};
    book.insert(o1, 1);

    EXPECT_DOUBLE_EQ(book.best_bid().to_double(), 100.50);

    // Cancel order
    bool res = book.cancel(OrderId(1));
    EXPECT_TRUE(res);
    book.verify_invariants();

    // Level should be cleared and deleted
    EXPECT_DOUBLE_EQ(book.best_bid().to_double(), 0.0);
    EXPECT_EQ(book.best_bid_level(), nullptr);

    // Verify listener logged cancellation & level removal
    ASSERT_EQ(listener.log.size(), 4);
    EXPECT_EQ(listener.log[2], "cancel_1");
    EXPECT_EQ(listener.log[3], "level_remove_10050000000_buy");
}

TEST(OrderBookTest, QuantityModification) {
    LimitOrderBook book;

    Order o1{OrderId(1), Price::from_double(100.50), Quantity::from_double(10), 101, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1), Side::Buy, OrderType::Limit, TimeInForce::GTC};
    Order o2{OrderId(2), Price::from_double(100.50), Quantity::from_double(5), 102, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1), Side::Buy, OrderType::Limit, TimeInForce::GTC};

    book.insert(o1, 1);
    book.insert(o2, 2);

    // 1. Shrink quantity (should retain head priority)
    bool mod1 = book.modify_qty(OrderId(1), Quantity::from_double(4), 3);
    EXPECT_TRUE(mod1);
    EXPECT_EQ(book.best_bid_level()->orders.head()->key.order_id, OrderId(1));
    EXPECT_DOUBLE_EQ(book.best_bid_level()->orders.head()->remaining_qty.to_double(), 4.0);

    // 2. Grow quantity (should lose priority and move to tail)
    bool mod2 = book.modify_qty(OrderId(1), Quantity::from_double(8), 4);
    EXPECT_TRUE(mod2);
    // Order 2 is now head, Order 1 is at the tail
    EXPECT_EQ(book.best_bid_level()->orders.head()->key.order_id, OrderId(2));
    EXPECT_EQ(book.best_bid_level()->orders.tail()->key.order_id, OrderId(1));
}

TEST(OrderBookTest, OrderFills) {
    MockOrderBookListener listener;
    LimitOrderBook book(&listener);

    Order o1{OrderId(1), Price::from_double(100.50), Quantity::from_double(10), 101, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1), Side::Buy, OrderType::Limit, TimeInForce::GTC};
    LiveOrder* live_order = book.insert(o1, 1);

    // 1. Partial fill
    book.fill_order(live_order, Quantity::from_double(3));
    book.verify_invariants();
    EXPECT_DOUBLE_EQ(live_order->remaining_qty.to_double(), 7.0);
    EXPECT_DOUBLE_EQ(book.best_bid_level()->total_qty.to_double(), 7.0);

    // 2. Full fill (removes order and level)
    book.fill_order(live_order, Quantity::from_double(7));
    book.verify_invariants();

    EXPECT_DOUBLE_EQ(book.best_bid().to_double(), 0.0);
    EXPECT_EQ(book.best_bid_level(), nullptr);

    // Verify fill log
    ASSERT_EQ(listener.log.size(), 5);
    EXPECT_EQ(listener.log[2], "fill_1_3000000");
    EXPECT_EQ(listener.log[3], "fill_1_7000000");
    EXPECT_EQ(listener.log[4], "level_remove_10050000000_buy");
}
