#pragma once

#include "GuiTypes.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace rtype::gui::detail {

using ColorRow = std::array<float, 3>;
using ColorMatrix = std::array<ColorRow, 3>;

inline ColorMatrix assistanceMatrix(ColorVisionMode mode, float strength)
{
    ColorMatrix result{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    if (mode == ColorVisionMode::Off)
        return result;

    constexpr std::array<ColorMatrix, 3> simulation{{
        {{{0.152286F, 1.052583F, -0.204868F},
          {0.114503F, 0.786281F, 0.099216F},
          {-0.003882F, -0.048116F, 1.051998F}}},
        {{{0.367322F, 0.860646F, -0.227968F},
          {0.280085F, 0.672501F, 0.047413F},
          {-0.011820F, 0.042940F, 0.968881F}}},
        {{{1.255528F, -0.076749F, -0.178779F},
          {-0.078411F, 0.930809F, 0.147602F},
          {0.004733F, 0.691367F, 0.303900F}}},
    }};
    const auto axis = static_cast<std::size_t>(mode) - 1;
    const auto& model = simulation.at(axis);

    for (std::size_t row = 0; row < result.size(); ++row) {
        if (row == axis)
            continue;
        for (std::size_t channel = 0; channel < result[row].size(); ++channel) {
            const float axisError = (channel == axis ? 1.0F : 0.0F) - model[axis][channel];
            const float rowError = (channel == row ? 1.0F : 0.0F) - model[row][channel];
            result[row][channel] += strength * (rowError + 0.7F * axisError);
        }
    }
    return result;
}

inline GuiColor previewColor(GuiColor color, const ColorMatrix& matrix)
{
    const auto linear = [](std::uint8_t byte) {
        const auto v = static_cast<float>(byte) / 255.0F;
        return v <= 0.04045F ? v / 12.92F : std::pow((v + 0.055F) / 1.055F, 2.4F);
    };
    const ColorRow rgb{linear(color.red), linear(color.green), linear(color.blue)};
    const auto encode = [&](const ColorRow& row) {
        const auto v = std::clamp(row[0] * rgb[0] + row[1] * rgb[1] + row[2] * rgb[2], 0.0F, 1.0F);
        const auto srgb = v <= 0.0031308F ? 12.92F * v : 1.055F * std::pow(v, 1.0F / 2.4F) - 0.055F;
        return static_cast<std::uint8_t>(std::lround(srgb * 255.0F));
    };
    return {encode(matrix[0]), encode(matrix[1]), encode(matrix[2]), color.alpha};
}

} // namespace rtype::gui::detail
