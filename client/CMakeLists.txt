add_executable(r-type_client
    src/main.cpp
    src/ClientSession.cpp
    src/ClientNetwork.cpp
    src/ClientSync.cpp
    src/ClientEntityFactory.cpp
    src/ClientWorld.cpp
)
target_include_directories(r-type_client PRIVATE src)
target_link_libraries(r-type_client PRIVATE rtype::protocol rtype::network rtype::gui rtype::engine)
rtype_set_warnings(r-type_client)
