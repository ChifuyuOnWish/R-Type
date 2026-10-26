add_executable(r-type_server
    src/main.cpp
    src/GameSession.cpp
    src/Players.cpp
    src/Replicator.cpp
    src/Spawner.cpp
    src/modes/InfiniteMode.cpp
    src/level/LevelLoader.cpp
    src/level/WaveDirector.cpp
    src/modes/ShipRules.cpp
    src/modes/LevelMode.cpp
)
target_include_directories(r-type_server PRIVATE include src src/modes)
target_link_libraries(r-type_server PRIVATE rtype::protocol rtype::engine rtype::network nlohmann_json::nlohmann_json)
rtype_set_warnings(r-type_server)
