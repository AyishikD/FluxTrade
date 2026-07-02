#pragma once

#include <domain/price.hpp>
#include <domain/quantity.hpp>

namespace fluxtrade {

struct alignas(8) PriceLevel {
    Price price;
    Quantity aggregated_qty;
    uint32_t order_count = 0;
    uint32_t padding = 0; // Pad to 8 bytes boundary
};

static_assert(sizeof(PriceLevel) == 24, "PriceLevel size must be exactly 24 bytes");
static_assert(alignof(PriceLevel) == 8, "PriceLevel alignment must be 8 bytes");

} // namespace fluxtrade
