#pragma once

#include <gateway/client_connection.hpp>
#include <gateway/protocol.hpp>
#include <cstddef>

namespace fluxtrade {

class SessionManager {
public:
    SessionManager() noexcept {
        reset();
    }
    ~SessionManager() = default;

    [[nodiscard]] ClientConnection* get_connection(int32_t conn_id) noexcept {
        for (size_t i = 0; i < active_count_; ++i) {
            if (connections_[i].conn_id == conn_id) {
                return &connections_[i];
            }
        }
        return nullptr;
    }

    [[nodiscard]] ClientConnection* create_connection(int32_t conn_id) noexcept {
        if (active_count_ >= MAX_CONNECTIONS) {
            return nullptr;
        }
        
        ClientConnection* conn = get_connection(conn_id);
        if (conn) return conn;

        conn = &connections_[active_count_++];
        *conn = ClientConnection{};
        conn->conn_id = conn_id;
        return conn;
    }

    void remove_connection(int32_t conn_id) noexcept {
        for (size_t i = 0; i < active_count_; ++i) {
            if (connections_[i].conn_id == conn_id) {
                if (i != active_count_ - 1) {
                    connections_[i] = connections_[active_count_ - 1];
                }
                active_count_--;
                return;
            }
        }
    }

    [[nodiscard]] bool authenticate(
        int32_t conn_id,
        uint32_t client_id,
        uint32_t account_id,
        const char* username,
        const char* password
    ) noexcept {
        ClientConnection* conn = get_connection(conn_id);
        if (!conn) return false;

        if (username[0] == '\0' || password[0] == '\0') {
            return false;
        }

        conn->client_id = client_id;
        conn->account_id = account_id;
        conn->authenticated = true;
        conn->last_heartbeat_ts = 0;
        return true;
    }

    void reset() noexcept {
        active_count_ = 0;
        for (size_t i = 0; i < MAX_CONNECTIONS; ++i) {
            connections_[i] = ClientConnection{};
        }
    }

    [[nodiscard]] size_t active_count() const noexcept { return active_count_; }
    [[nodiscard]] const ClientConnection* connections() const noexcept { return connections_; }

private:
    static constexpr size_t MAX_CONNECTIONS = 64;
    ClientConnection connections_[MAX_CONNECTIONS];
    size_t active_count_ = 0;
};

} // namespace fluxtrade
