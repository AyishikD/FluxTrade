#include <gtest/gtest.h>
#include <domain/ids.hpp>
#include <domain/price.hpp>
#include <domain/quantity.hpp>
#include <domain/order.hpp>
#include <domain/order_view.hpp>
#include <domain/trade.hpp>
#include <domain/execution.hpp>
#include <domain/fill.hpp>
#include <domain/price_level.hpp>
#include <domain/concepts.hpp>
#include <domain/validators.hpp>

using namespace fluxtrade;

// 1. Static Layout & Concept Constraints
TEST(DomainTest, StaticConstraints) {
    // Concept validations
    static_assert(StrongIdentifier<OrderId>);
    static_assert(StrongIdentifier<SymbolId>);
    static_assert(DomainEntity<Order>);
    static_assert(DomainEntity<Trade>);
    static_assert(DomainEntity<Execution>);
    static_assert(DomainEntity<Fill>);
    static_assert(DomainEntity<PriceLevel>);

    // Size constraints (cache line optimization)
    static_assert(sizeof(Order) == 64);
    static_assert(sizeof(Trade) == 56);
    static_assert(sizeof(Execution) == 56);
    static_assert(sizeof(Fill) == 48);
    static_assert(sizeof(PriceLevel) == 24);

    // Alignment checks
    static_assert(alignof(Order) == 8);
    static_assert(alignof(Trade) == 8);
    static_assert(alignof(Execution) == 8);
    static_assert(alignof(Fill) == 8);
    static_assert(alignof(PriceLevel) == 8);

    SUCCEED();
}

// 2. Strong Identifier Type-Safety
TEST(DomainTest, StrongIdentifiers) {
    OrderId o1(100);
    OrderId o2(100);
    OrderId o3(200);

    EXPECT_EQ(o1, o2);
    EXPECT_NE(o1, o3);
    EXPECT_LT(o1, o3);
    EXPECT_GT(o3, o1);

    // Verify raw values
    EXPECT_EQ(o1.get(), 100ULL);

    // Verify OrderKey
    SymbolId s(1);
    OrderKey k1{s, o1};
    OrderKey k2{s, o2};
    OrderKey k3{s, o3};

    EXPECT_EQ(k1, k2);
    EXPECT_NE(k1, k3);
    EXPECT_LT(k1, k3);
}

// 3. Price Fixed-Precision Math
TEST(DomainTest, PricePrecision) {
    Price p1 = Price::from_double(10.50);
    Price p2 = Price::from_double(5.25);

    // Tick validations
    EXPECT_EQ(p1.ticks(), 1050000000LL);
    EXPECT_DOUBLE_EQ(p1.to_double(), 10.50);

    // Math operations
    Price sum = p1 + p2;
    EXPECT_DOUBLE_EQ(sum.to_double(), 15.75);

    Price diff = p1 - p2;
    EXPECT_DOUBLE_EQ(diff.to_double(), 5.25);

    p1 += p2;
    EXPECT_DOUBLE_EQ(p1.to_double(), 15.75);
}

// 4. Quantity Precision Math
TEST(DomainTest, QuantityPrecision) {
    Quantity q1 = Quantity::from_double(2.5);
    Quantity q2 = Quantity::from_double(1.25);

    EXPECT_EQ(q1.units(), 2500000ULL);
    EXPECT_DOUBLE_EQ(q1.to_double(), 2.5);

    Quantity sum = q1 + q2;
    EXPECT_DOUBLE_EQ(sum.to_double(), 3.75);

    q1 -= q2;
    EXPECT_DOUBLE_EQ(q1.to_double(), 1.25);
}

// 5. Validators Verification
TEST(DomainTest, Validators) {
    EXPECT_TRUE(is_valid_price(Price(1)));
    EXPECT_FALSE(is_valid_price(Price(0)));
    EXPECT_FALSE(is_valid_price(Price(-1)));

    EXPECT_TRUE(is_valid_quantity(Quantity(1)));
    EXPECT_FALSE(is_valid_quantity(Quantity(0)));

    EXPECT_TRUE(is_valid_side(Side::Buy));
    EXPECT_TRUE(is_valid_side(Side::Sell));

    EXPECT_TRUE(is_valid_order_type(OrderType::Limit));
    EXPECT_TRUE(is_valid_order_type(OrderType::Market));

    EXPECT_TRUE(is_valid_time_in_force(TimeInForce::GTC));
    EXPECT_TRUE(is_valid_time_in_force(TimeInForce::IOC));
}
