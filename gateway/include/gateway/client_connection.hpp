#pragma once

#include <cstdint>
#include <cstddef>

namespace fluxtrade {

struct ClientConnection {
    int32_t conn_id = -1;
    uint32_t client_id = 0;
    uint32_t account_id = 0;
    bool authenticated = false;
    uint64_t last_heartbeat_ts = 0;
    uint64_t next_inbound_seq = 1;
    uint64_t next_outbound_seq = 1;
    
    // Fixed TCP framing read buffer
    static constexpr size_t BUFFER_SIZE = 8192;
    uint8_t buffer[BUFFER_SIZE] = {0};
    size_t buffer_len = 0;
};

} // namespace fluxtrade
