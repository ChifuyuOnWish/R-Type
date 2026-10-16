# Public presentation contract; concrete rendering is implemented separately.
add_library(rtype_gui INTERFACE)
add_library(rtype::gui ALIAS rtype_gui)

target_include_directories(rtype_gui INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_compile_features(rtype_gui INTERFACE cxx_std_20)
target_link_libraries(rtype_gui INTERFACE raylib)
