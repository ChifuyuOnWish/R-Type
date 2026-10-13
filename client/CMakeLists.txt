add_executable(r-type_client
    src/main.cpp
)
target_link_libraries(r-type_client PRIVATE rtype::protocol)
rtype_set_warnings(r-type_client)
