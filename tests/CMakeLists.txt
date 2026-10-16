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

# ByteWriter / ByteReader (issue #3)
add_executable(rtype_serialization_test serialization_test.cpp)
target_link_libraries(rtype_serialization_test PRIVATE rtype::network)
add_test(NAME serialization COMMAND rtype_serialization_test)