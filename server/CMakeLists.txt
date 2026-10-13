add_executable(r-type_server
    src/main.cpp
)
target_link_libraries(r-type_server PRIVATE rtype::protocol)
rtype_set_warnings(r-type_server)
