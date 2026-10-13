add_library(rtype_network STATIC
    src/placeholder.cpp
)
add_library(rtype::network ALIAS rtype_network)

target_include_directories(rtype_network PUBLIC include)

target_link_libraries(rtype_network PUBLIC asio::asio)

rtype_set_warnings(rtype_network)
