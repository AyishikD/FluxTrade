#include <benchmark/benchmark.h>
#include <gateway/packet_encoder.hpp>
#include <gateway/packet_decoder.hpp>
#include <gateway/sequence_manager.hpp>
#include <exchange/dispatcher.hpp>

using namespace fluxtrade;

// Benchmark Binary Packet Encoding (<= 15 ns)
static void BM_Gateway_PacketEncode(benchmark::State& state) {
    uint8_t buffer[512];
    NewOrderPacket ord{};
    ord.client_order_id = 1001;
    ord.symbol_id = 1;
    ord.account_id = 99;
    ord.price_ticks = 10050;
    ord.qty_units = 1000;
    ord.side = 0;

    for (auto _ : state) {
        size_t len = PacketEncoder::encode(buffer, sizeof(buffer), MessageType::NewOrder, 1, 1000, ord);
        benchmark::DoNotOptimize(len);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Gateway_PacketEncode);

// Benchmark Binary Packet Decoding (<= 15 ns)
static void BM_Gateway_PacketDecode(benchmark::State& state) {
    uint8_t buffer[512];
    NewOrderPacket ord{};
    ord.client_order_id = 1001;
    size_t len = PacketEncoder::encode(buffer, sizeof(buffer), MessageType::NewOrder, 1, 1000, ord);
    
    MessageHeader header;
    const uint8_t* payload = nullptr;

    for (auto _ : state) {
        auto bytes = PacketDecoder::decode(std::span<const uint8_t>(buffer, len), header, payload);
        benchmark::DoNotOptimize(bytes);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Gateway_PacketDecode);

// Benchmark Sequence Assignment (<= 2 ns)
static void BM_Gateway_SequenceAssignment(benchmark::State& state) {
    SequenceManager manager;

    for (auto _ : state) {
        uint64_t seq = manager.next_sequence();
        benchmark::DoNotOptimize(seq);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Gateway_SequenceAssignment);

// Benchmark Dispatcher Routing (<= 10 ns)
static void BM_Gateway_DispatcherRouting(benchmark::State& state) {
    Dispatcher<32, 4096> dispatcher;
    OrderCommand cmd;
    cmd.symbol_id = SymbolId(1);
    cmd.client_order_id = 1001;

    for (auto _ : state) {
        // Pop to prevent queue saturation overhead inside loops
        auto& queue = dispatcher.get_queue(1);
        if (queue.size() >= 4000) {
            OrderCommand tmp;
            for (size_t i = 0; i < 2000; ++i) {
                queue.pop(tmp);
            }
        }
        bool ok = dispatcher.dispatch(cmd);
        benchmark::DoNotOptimize(ok);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Gateway_DispatcherRouting);

BENCHMARK_MAIN();
