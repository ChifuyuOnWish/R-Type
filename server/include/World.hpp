#pragma once

namespace rtype::server {

inline constexpr float WorldWidth = 1920.F;
inline constexpr float WorldHeight = 1080.F;

inline constexpr float CeilingHeight = 60.F;
inline constexpr float GroundHeight = 60.F;
inline constexpr float PlayTop = CeilingHeight;
inline constexpr float PlayBottom = WorldHeight - GroundHeight;

} // namespace rtype::server
