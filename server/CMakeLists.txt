add_executable(r-type_server
    src/main.cpp
    src/ServerGame.cpp
)
target_link_libraries(r-type_server PRIVATE rtype::protocol rtype::engine)
rtype_set_warnings(r-type_server)