#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace rtype::protocol {

struct Fixed
{
    static constexpr float Scale = 256.0f;

    std::int32_t raw = 0;

    static Fixed fromFloat(float v)
    {
        const float scaled = std::clamp(std::round(v * Scale), -2147483648.0f, 2147483520.0f);
        return Fixed{static_cast<std::int32_t>(scaled)};
    }
    float toFloat() const { return static_cast<float>(raw) / Scale; }

    bool operator==(const Fixed&) const = default;
};

struct Fixed16
{
    static constexpr float Scale = 256.0f;

    std::int16_t raw = 0;

    static Fixed16 fromFloat(float v)
    {
        const float scaled = std::clamp(std::round(v * Scale), -32768.0f, 32767.0f);
        return Fixed16{static_cast<std::int16_t>(scaled)};
    }
    float toFloat() const { return static_cast<float>(raw) / Scale; }

    bool operator==(const Fixed16&) const = default;
};

} // namespace rtype::protocol