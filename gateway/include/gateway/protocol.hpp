#pragma once

#include <cstdint>
#include <cstddef>

namespace fluxtrade {

enum class MessageType : uint16_t {
    Heartbeat        = 1,
    LoginRequest     = 2,
    LoginResponse    = 3,
    Logout           = 4,
    NewOrder         = 5,
    CancelOrder      = 6,
    ReplaceOrder     = 7,
    ExecutionReport  = 8,
    Reject           = 9,
    Trade            = 10,
    BookUpdate       = 11
};

struct alignas(8) MessageHeader {
    uint16_t length;
    uint16_t type;
    uint32_t padding = 0;
    uint64_t sequence;
    uint64_t timestamp;
};

static_assert(sizeof(MessageHeader) == 24, "MessageHeader must be exactly 24 bytes");

struct alignas(8) HeartbeatPacket {
    MessageHeader header;
};

struct alignas(8) LoginRequestPacket {
    MessageHeader header;
    uint32_t client_id;
    uint32_t account_id;
    char username[16];
    char password[16];
};

struct alignas(8) LoginResponsePacket {
    MessageHeader header;
    uint32_t client_id;
    uint32_t account_id;
    uint8_t success;
    char reject_reason[23];
};

struct alignas(8) LogoutPacket {
    MessageHeader header;
    uint32_t client_id;
    uint32_t padding;
};

struct alignas(8) NewOrderPacket {
    MessageHeader header;
    uint64_t client_order_id;
    uint32_t symbol_id;
    uint32_t account_id;
    uint64_t price_ticks;
    uint64_t qty_units;
    uint8_t side; // 0 = Buy, 1 = Sell
    uint8_t type; // 0 = Limit, 1 = Market
    uint8_t tif;  // TimeInForce
    uint8_t padding[5] = {0, 0, 0, 0, 0};
};

struct alignas(8) CancelOrderPacket {
    MessageHeader header;
    uint64_t client_order_id;
    uint64_t orig_client_order_id;
    uint32_t symbol_id;
    uint32_t account_id;
};

struct alignas(8) ReplaceOrderPacket {
    MessageHeader header;
    uint64_t client_order_id;
    uint64_t orig_client_order_id;
    uint32_t symbol_id;
    uint32_t account_id;
    uint64_t price_ticks;
    uint64_t qty_units;
};

struct alignas(8) ExecutionReportPacket {
    MessageHeader header;
    uint64_t order_id;
    uint64_t client_order_id;
    uint32_t symbol_id;
    uint32_t account_id;
    uint64_t price_ticks;
    uint64_t qty_units;
    uint64_t filled_qty;
    uint64_t remaining_qty;
    uint8_t side;
    uint8_t status; // ExecType
    uint8_t padding[6] = {0, 0, 0, 0, 0, 0};
};

struct alignas(8) RejectPacket {
    MessageHeader header;
    uint64_t client_order_id;
    uint32_t account_id;
    uint16_t reason;
    uint8_t padding[6] = {0, 0, 0, 0, 0, 0};
};

} // namespace fluxtrade
