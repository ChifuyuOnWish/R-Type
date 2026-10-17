# rtype_protocol: R-Type message types and their (de)serialization,
# used by the server and the client, built on top of rtype_network.
add_library(rtype_protocol STATIC
    src/Messages.cpp
)
add_library(rtype::protocol ALIAS rtype_protocol)

# Same layout as network/: global headers in include/, class headers next to their .cpp.
target_include_directories(rtype_protocol PUBLIC include src)
target_link_libraries(rtype_protocol PUBLIC rtype::network)

rtype_set_warnings(rtype_protocol)
