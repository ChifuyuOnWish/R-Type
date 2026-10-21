add_executable(r-type_server
    src/main.cpp
    src/GameSession.cpp
    src/Players.cpp
    src/Replicator.cpp
    src/Spawner.cpp
    src/modes/InfiniteMode.cpp
)
target_include_directories(r-type_server PRIVATE include src src/modes)
target_link_libraries(r-type_server PRIVATE rtype::protocol rtype::engine)
rtype_set_warnings(r-type_server)
