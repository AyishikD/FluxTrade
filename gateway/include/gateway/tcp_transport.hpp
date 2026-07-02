#pragma once

#include <gateway/transport.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <cerrno>

namespace fluxtrade {

class TcpTransport : public ITransport {
public:
    TcpTransport() = default;
    virtual ~TcpTransport() { stop(); }

    bool start(const char*, uint16_t port) override {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) return false;

        const int flags = fcntl(server_fd_, F_GETFL, 0);
        fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);

        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(server_fd_);
            server_fd_ = -1;
            return false;
        }

        if (listen(server_fd_, 128) < 0) {
            close(server_fd_);
            server_fd_ = -1;
            return false;
        }

        return true;
    }

    void stop() override {
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
        for (int fd : clients_) {
            close(fd);
        }
        clients_.clear();
    }

    void poll() override {
        if (server_fd_ < 0) return;

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd >= 0) {
            const int flags = fcntl(client_fd, F_GETFL, 0);
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
            clients_.push_back(client_fd);
            if (listener_) {
                listener_->on_connect(client_fd);
            }
        }

        std::vector<int> disconnected;
        uint8_t buffer[4096];
        for (int fd : clients_) {
            const ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
            if (n > 0) {
                if (listener_) {
                    listener_->on_read(fd, std::span<const uint8_t>(buffer, n));
                }
            } else if (n == 0) {
                disconnected.push_back(fd);
            } else {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    disconnected.push_back(fd);
                }
            }
        }

        for (int fd : disconnected) {
            auto it = std::find(clients_.begin(), clients_.end(), fd);
            if (it != clients_.end()) {
                clients_.erase(it);
            }
            if (listener_) {
                listener_->on_disconnect(fd);
            }
            close(fd);
        }
    }

    bool send(int32_t conn_id, std::span<const uint8_t> data) override {
        const ssize_t n = write(conn_id, data.data(), data.size());
        return n == static_cast<ssize_t>(data.size());
    }

    void disconnect(int32_t conn_id) override {
        auto it = std::find(clients_.begin(), clients_.end(), conn_id);
        if (it != clients_.end()) {
            clients_.erase(it);
        }
        if (listener_) {
            listener_->on_disconnect(conn_id);
        }
        close(conn_id);
    }

    void set_listener(ITransportListener* listener) override {
        listener_ = listener;
    }

private:
    int server_fd_ = -1;
    std::vector<int> clients_;
    ITransportListener* listener_ = nullptr;
};

} // namespace fluxtrade
