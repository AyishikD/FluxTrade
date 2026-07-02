#pragma once

#include <unordered_map>
#include <typeindex>
#include <string>

namespace fluxtrade {

class ServiceRegistry {
public:
    ServiceRegistry() = default;
    ~ServiceRegistry() = default;

    ServiceRegistry(const ServiceRegistry&) = delete;
    ServiceRegistry& operator=(const ServiceRegistry&) = delete;
    ServiceRegistry(ServiceRegistry&&) = delete;
    ServiceRegistry& operator=(ServiceRegistry&&) = delete;

    // Registers a service pointer mapped to its type index.
    template <typename T>
    void register_service(T* service) noexcept {
        services_[std::type_index(typeid(T))] = static_cast<void*>(service);
    }

    // Resolves and returns a typed service pointer. Returns nullptr if not registered.
    template <typename T>
    [[nodiscard]] T* get_service() const noexcept {
        auto it = services_.find(std::type_index(typeid(T)));
        if (it != services_.end()) {
            return static_cast<T*>(it->second);
        }
        return nullptr;
    }

private:
    std::unordered_map<std::type_index, void*> services_;
};

} // namespace fluxtrade
