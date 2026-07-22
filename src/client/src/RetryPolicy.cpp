// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/RetryPolicy.h"

#include <algorithm>
#include <cmath>

namespace QtOpenAi {
namespace Client {

int RetryPolicy::backoffDelayMs(int attempt) const
{
    if (initialDelayMs <= 0)
        return 0;
    const double raw = initialDelayMs * std::pow(multiplier, std::max(0, attempt));
    const double capped = std::min<double>(raw, maxDelayMs > 0 ? maxDelayMs : raw);
    return static_cast<int>(capped);
}

} // namespace Client
} // namespace QtOpenAi
