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

# Engine Update (issue #38)
add_executable(rtype_engine_update_test engine_update_test.cpp)
target_link_libraries(rtype_engine_update_test PRIVATE rtype::engine)
rtype_set_warnings(rtype_engine_update_test)
add_test(NAME game_loop COMMAND rtype_engine_update_test)

# NetworkClient against a NetworkServer on loopback, and failure paths (issue #13)
add_executable(rtype_network_client_test network_client_test.cpp)
target_link_libraries(rtype_network_client_test PRIVATE rtype::network)
rtype_set_warnings(rtype_network_client_test)
add_test(NAME network_client COMMAND rtype_network_client_test)
set_tests_properties(network_client PROPERTIES TIMEOUT 30)
# Engine Motion, Input and Bounds systems (issue #41 and #40)
add_executable(rtype_engine_systems_test engine_systems_test.cpp)
target_link_libraries(rtype_engine_systems_test PRIVATE rtype::engine)
rtype_set_warnings(rtype_engine_systems_test)
add_test(NAME engine_systems COMMAND rtype_engine_systems_test)

# Action mapping runs on injected keyboard snapshots without a graphics context.
add_executable(rtype_input_mapper_test InputMapperTest.cpp)
target_link_libraries(rtype_input_mapper_test PRIVATE rtype::gui)
rtype_set_warnings(rtype_input_mapper_test)
add_test(NAME input_mapper COMMAND rtype_input_mapper_test)

# Compile the audio implementation against a fake backend, without a physical device.
add_executable(rtype_audio_test AudioSystemTest.cpp ../gui/src/AudioSystem.cpp)
target_include_directories(rtype_audio_test PRIVATE ../gui/include ../gui/src)
target_include_directories(rtype_audio_test SYSTEM PRIVATE
    $<TARGET_PROPERTY:raylib,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_features(rtype_audio_test PRIVATE cxx_std_20)
rtype_set_warnings(rtype_audio_test)
add_test(NAME audio COMMAND rtype_audio_test)

# Status layouts and message handling do not require a graphics context.
add_executable(rtype_status_presenter_test StatusPresenterTest.cpp)
target_link_libraries(rtype_status_presenter_test PRIVATE rtype::gui)
rtype_set_warnings(rtype_status_presenter_test)
add_test(NAME status_presenter COMMAND rtype_status_presenter_test)

# Engine EventDispatcher (issue #39)
add_executable(rtype_engine_events_test engine_events_test.cpp)
target_link_libraries(rtype_engine_events_test PRIVATE rtype::engine)
rtype_set_warnings(rtype_engine_events_test)
add_test(NAME engine_events COMMAND rtype_engine_events_test)
