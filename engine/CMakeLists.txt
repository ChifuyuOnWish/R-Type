add_library(rtype_engine STATIC
    src/ecs/Entity.cpp
)
add_library(rtype::engine ALIAS rtype_engine)

target_include_directories(rtype_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_compile_features(rtype_engine PUBLIC cxx_std_20)

rtype_set_warnings(rtype_engine)
