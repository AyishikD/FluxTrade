#pragma once

#include <cstdint>

namespace fluxtrade {

using EventVersion = uint16_t;

struct ProtocolVersion {
    static constexpr EventVersion V1 = 1;
    static constexpr EventVersion Current = V1;
};

} // namespace fluxtrade
