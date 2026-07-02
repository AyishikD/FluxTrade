#include <gtest/gtest.h>
#include <persistence/replay_engine.hpp>
#include <persistence/memory_journal.hpp>
#include <persistence/disk_journal.hpp>
#include <persistence/snapshot_writer.hpp>
#include <matching/order_book/order_book.hpp>
#include <gateway/order_command.hpp>
#include <string>
#include <filesystem>

using namespace fluxtrade;
using namespace fluxtrade::persistence;

TEST(ReplayTests, CompleteRecoveryFromJournalAndSnapshot) {
    std::string test_dir = "test_replay_dir";
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);
    
    // 1. Create a Snapshot for sequence 1
    LimitOrderBook initial_book;
    Order o1{
        .order_id = OrderId(1),
        .price = Price::from_double(100.00),
        .qty = Quantity(10),
        .seq_num = 1,
        .symbol_id = SymbolId(1),
        .side = Side::Buy,
        .type = OrderType::Limit
    };
    initial_book.insert(o1, 1);
    
    SnapshotWriter::save(test_dir + "/snapshot_1.bin", initial_book, 1, 1000);
    
    MetadataManager meta(test_dir);
    MetadataInfo info{};
    info.latest_snapshot_sequence = 1;
    info.last_journal_sequence = 2;
    info.current_journal_index = 1;
    meta.save(info);
    
    // 2. Create a WAL journal (journal_000001.wal) starting from seq 2
    RingBuffer<LogEntry, 1024> queue;
    DiskJournal journal(test_dir, FlushPolicy::Immediate, 1024 * 1024);
    
    // We hack start to make it write journal_000001.wal directly
    journal.start(queue);
    
    OrderCommand cmd{};
    cmd.type = CommandType::NewOrder;
    cmd.sequence = 2;
    cmd.timestamp = 2000;
    cmd.client_order_id = 2;
    cmd.symbol_id = SymbolId(1);
    cmd.account_id = AccountId(1);
    cmd.price = Price::from_double(101.00);
    cmd.qty = Quantity(20);
    cmd.side = Side::Sell;
    
    MemoryJournal::append(queue, 2, 2000, static_cast<uint16_t>(cmd.type), &cmd, sizeof(OrderCommand));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    journal.stop();
    
    // 3. Replay
    ReplayEngine replay(test_dir);
    LimitOrderBook recovered_book;
    
    bool recovered = replay.recover(recovered_book);
    EXPECT_TRUE(recovered);
    
    // 4. Verify
    EXPECT_EQ(recovered_book.best_bid(), Price::from_double(100.00));
    EXPECT_EQ(recovered_book.best_ask(), Price::from_double(101.00));
    
    auto* ask_level = recovered_book.best_ask_level();
    ASSERT_NE(ask_level, nullptr);
    EXPECT_EQ(ask_level->total_qty, Quantity(20));
    
    std::filesystem::remove_all(test_dir);
}
