add_executable(r-type_client
    src/main.cpp
    src/ClientNetwork.cpp
    src/ClientSync.cpp
    src/ClientEntityFactory.cpp
    src/ClientWorld.cpp
)
target_include_directories(r-type_client PRIVATE src)
# rtype::network : adapte le nom si ta cible réseau s'appelle autrement.
# NetworkClient.hpp est dans network/src : la cible réseau doit exposer ce dossier
# en PUBLIC (target_include_directories(rtype_network PUBLIC src include)).
target_link_libraries(r-type_client PRIVATE rtype::protocol rtype::network rtype::gui rtype::engine)
rtype_set_warnings(r-type_client)