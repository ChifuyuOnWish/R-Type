add_library(rtype_gui STATIC
    src/Window.cpp
    src/Gui.cpp
    src/ResourceManager.cpp
    src/Renderer.cpp
    src/ScrollingBackground.cpp
    src/InputMapper.cpp
    src/AudioSystem.cpp
)
add_library(rtype::gui ALIAS rtype_gui)

target_include_directories(rtype_gui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_compile_features(rtype_gui PUBLIC cxx_std_20)
target_link_libraries(rtype_gui PRIVATE raylib)

rtype_set_warnings(rtype_gui)
