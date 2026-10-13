add_library(rtype_protocol STATIC
    src/placeholder.cpp
)
add_library(rtype::protocol ALIAS rtype_protocol)

target_include_directories(rtype_protocol PUBLIC include)
target_link_libraries(rtype_protocol PUBLIC rtype::network)

rtype_set_warnings(rtype_protocol)
