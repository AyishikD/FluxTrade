#include <gtest/gtest.h>
#include <persistence/snapshot_writer.hpp>
#include <persistence/snapshot_reader.hpp>
#include <matching/order_book/order_book.hpp>
#include <string>
#include <filesystem>

using namespace fluxtrade;
using namespace fluxtrade::persistence;

TEST(SnapshotTests, WriteAndReadBasicBook) {
    LimitOrderBook book;
    
    Order o1{
        .order_id = OrderId(1),
        .price = Price::from_double(100.50),
        .qty = Quantity(100),
        .seq_num = 1,
        .symbol_id = SymbolId(1),
        .side = Side::Buy,
        .type = OrderType::Limit
    };
    
    Order o2{
        .order_id = OrderId(2),
        .price = Price::from_double(101.00),
        .qty = Quantity(50),
        .seq_num = 2,
        .symbol_id = SymbolId(1),
        .side = Side::Sell,
        .type = OrderType::Limit
    };
    
    book.insert(o1, 1);
    book.insert(o2, 2);
    
    std::string snap_file = "test_snapshot.bin";
    EXPECT_TRUE(SnapshotWriter::save(snap_file, book, 2, 1000));
    
    LimitOrderBook restored_book;
    uint64_t seq = 0;
    uint64_t ts = 0;
    EXPECT_TRUE(SnapshotReader::load(snap_file, restored_book, seq, ts));
    
    EXPECT_EQ(seq, 2);
    EXPECT_EQ(ts, 1000);
    EXPECT_EQ(restored_book.best_bid(), Price::from_double(100.50));
    EXPECT_EQ(restored_book.best_ask(), Price::from_double(101.00));
    
    auto* bid_level = restored_book.best_bid_level();
    ASSERT_NE(bid_level, nullptr);
    EXPECT_EQ(bid_level->total_qty, Quantity(100));
    
    auto* ask_level = restored_book.best_ask_level();
    ASSERT_NE(ask_level, nullptr);
    EXPECT_EQ(ask_level->total_qty, Quantity(50));
    
    std::filesystem::remove(snap_file);
}
