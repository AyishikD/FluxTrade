#pragma once

#include <matching/matching_engine.hpp>
#include <common/expected.hpp>
#include <unordered_map>
#include <memory>
#include <string>

namespace fluxtrade {

class ExchangeSimulator {
public:
    ExchangeSimulator() = default;
    ~ExchangeSimulator() = default;

    ExchangeSimulator(const ExchangeSimulator&) = delete;
    ExchangeSimulator& operator=(const ExchangeSimulator&) = delete;

    // Registers a matching engine for a specific symbol
    expected<void, std::string> register_symbol(SymbolId symbol_id) noexcept {
        if (engines_.find(symbol_id.get()) != engines_.end()) {
            return unexpected<std::string>("Symbol with ID " + std::to_string(symbol_id.get()) + " already registered");
        }
        engines_[symbol_id.get()] = std::make_unique<MatchingEngine>();
        return {};
    }

    // Fetches the matching engine for a symbol
    [[nodiscard]] MatchingEngine* get_engine(SymbolId symbol_id) const noexcept {
        auto it = engines_.find(symbol_id.get());
        if (it == engines_.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    // Resets all symbol matching cores
    void reset() noexcept {
        for (auto& [id, engine] : engines_) {
            engine->reset();
        }
    }

private:
    std::unordered_map<uint32_t, std::unique_ptr<MatchingEngine>> engines_;
};

} // namespace fluxtrade
