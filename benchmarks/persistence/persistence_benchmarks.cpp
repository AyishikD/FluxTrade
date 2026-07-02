#include <benchmark/benchmark.h>
#include <persistence/wal_record.hpp>
#include <persistence/checksum.hpp>
#include <persistence/memory_journal.hpp>
#include <persistence/disk_journal.hpp>
#include <persistence/snapshot_writer.hpp>
#include <matching/order_book/order_book.hpp>
#include <gateway/order_command.hpp>
#include <vector>
#include <string>
#include <filesystem>

using namespace fluxtrade;
using namespace fluxtrade::persistence;

static void BM_Checksum(benchmark::State& state) {
    std::vector<std::byte> data(state.range(0));
    for (auto _ : state) {
        uint32_t crc = Checksum::crc32c(data.data(), data.size());
        benchmark::DoNotOptimize(crc);
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)));
}
BENCHMARK(BM_Checksum)->Range(64, 1024);

static void BM_MemoryJournalAppend(benchmark::State& state) {
    RingBuffer<LogEntry, 65536> queue;
    OrderCommand cmd{};
    cmd.type = CommandType::NewOrder;
    cmd.sequence = 1;
    cmd.timestamp = 1000;
    cmd.client_order_id = 1;
    cmd.symbol_id = SymbolId(1);
    cmd.account_id = AccountId(1);
    cmd.price = Price::from_double(100.00);
    cmd.qty = Quantity(10);
    cmd.side = Side::Buy;

    LogEntry dummy;
    for (auto _ : state) {
        MemoryJournal::append(queue, cmd.sequence, cmd.timestamp, static_cast<uint16_t>(cmd.type), &cmd, sizeof(OrderCommand));
        benchmark::DoNotOptimize(queue.pop(dummy)); // Keep queue empty
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemoryJournalAppend);

static void BM_SnapshotWriter(benchmark::State& state) {
    LimitOrderBook book;
    for (int i = 0; i < state.range(0); ++i) {
        Order o{
            .order_id = OrderId(i + 1),
            .price = Price::from_ticks(10000 + (i % 100)),
            .qty = Quantity(10),
            .seq_num = static_cast<uint64_t>(i + 1),
            .symbol_id = SymbolId(1),
            .side = (i % 2 == 0) ? Side::Buy : Side::Sell,
            .type = OrderType::Limit
        };
        book.insert(o, i + 1);
    }

    std::string snap_file = "benchmark_snapshot.bin";
    
    for (auto _ : state) {
        SnapshotWriter::save(snap_file, book, state.range(0), 1000);
    }
    
    std::filesystem::remove(snap_file);
    state.SetItemsProcessed(int64_t(state.iterations()) * state.range(0));
}
BENCHMARK(BM_SnapshotWriter)->RangeMultiplier(10)->Range(100, 100000);

BENCHMARK_MAIN();
