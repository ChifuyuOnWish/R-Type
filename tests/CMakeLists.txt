# Test binaries stay in the build tree; only the game binaries go to the repo root.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
foreach(config IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

# Placeholder test so CI runs ctest from day one.
# Replace with the test framework the team picks (added via CPM).
add_executable(rtype_smoke_test smoke_test.cpp)
target_link_libraries(rtype_smoke_test PRIVATE rtype::protocol)
add_test(NAME smoke COMMAND rtype_smoke_test)

# Exercise the public GUI contract without opening a window or audio device.
add_executable(rtype_gui_interface_test GuiInterfaceTest.cpp)
target_link_libraries(rtype_gui_interface_test PRIVATE rtype::gui)
rtype_set_warnings(rtype_gui_interface_test)
add_test(NAME gui_interface COMMAND rtype_gui_interface_test)

# ByteWriter / ByteReader (issue #3)
add_executable(rtype_serialization_test serialization_test.cpp)
target_link_libraries(rtype_serialization_test PRIVATE rtype::network)
add_test(NAME serialization COMMAND rtype_serialization_test)

# EntityManager / SparseSet / Registry (issues #36, #37)
add_executable(rtype_ecs_test ecs_test.cpp)
target_link_libraries(rtype_ecs_test PRIVATE rtype::engine)
rtype_set_warnings(rtype_ecs_test)
add_test(NAME ecs COMMAND rtype_ecs_test)

# Packet header validation and wrap-safe sequence comparisons.
add_executable(rtype_packet_header_test packet_header_test.cpp)
target_link_libraries(rtype_packet_header_test PRIVATE rtype::network)
add_test(NAME packet_header COMMAND rtype_packet_header_test)

# Resource error paths run without a window or audio device.
add_executable(rtype_resources_test ResourceManagerTest.cpp)
target_link_libraries(rtype_resources_test PRIVATE rtype::gui)
rtype_set_warnings(rtype_resources_test)
add_test(NAME resources COMMAND rtype_resources_test)

# Game messages round trips and rejections (issue #14)
add_executable(rtype_protocol_messages_test protocol_messages_test.cpp)
target_link_libraries(rtype_protocol_messages_test PRIVATE rtype::protocol)
add_test(NAME protocol_messages COMMAND rtype_protocol_messages_test)

# Background motion and wrapping run without a graphics context.
add_executable(rtype_background_test ScrollingBackgroundTest.cpp)
target_link_libraries(rtype_background_test PRIVATE rtype::gui)
rtype_set_warnings(rtype_background_test)
add_test(NAME background COMMAND rtype_background_test)

# Engine GameLoop (issue #38)
add_executable(rtype_engine_gameLoop_test engine_gameLoop_test.cpp)
target_link_libraries(rtype_engine_gameLoop_test PRIVATE rtype::engine)
rtype_set_warnings(rtype_engine_gameLoop_test)
add_test(NAME game_loop COMMAND rtype_engine_gameLoop_test)
