#pragma once

#include <cstdint>

namespace fluxtrade {

enum class ExecutionType : uint8_t {
    New    = 1,
    Trade  = 2,
    Cancel = 3,
    Reject = 4
};

} // namespace fluxtrade
