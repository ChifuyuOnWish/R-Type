# rtype_network: generic UDP networking code, built once and linked into both
# the server and the client. Independent of engine/ and gui/; only depends on Asio.
# Knows nothing about R-Type; game messages live in network/protocol.
add_library(rtype_network STATIC
    src/placeholder.cpp
    # Add each file as its issue lands, e.g.:
    # src/ByteWriter.cpp
    # src/ByteReader.cpp
    # src/PacketHeader.cpp
    # src/UdpSocket.cpp
    # src/Connection.cpp
    # src/NetworkHost.cpp
    # src/NetworkServer.cpp
    # src/NetworkClient.cpp
)
add_library(rtype::network ALIAS rtype_network)

# include/ holds global headers (constants, shared types); each class's .hpp sits
# next to its .cpp in src/. Both are PUBLIC so the server and client can include them.
# Flat names: #include "UdpSocket.hpp". Keep names unique across network/, engine/ and gui/.
target_include_directories(rtype_network PUBLIC include src)

# PUBLIC because NetworkHost's header exposes the socket and io_context.
target_link_libraries(rtype_network PUBLIC asio::asio)

rtype_set_warnings(rtype_network)

add_subdirectory(protocol)
