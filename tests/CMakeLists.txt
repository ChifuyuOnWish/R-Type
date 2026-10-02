# Placeholder test so CI runs ctest from day one.
# Replace with the test framework the team picks (added via CPM).
add_executable(rtype_smoke_test smoke_test.cpp)
add_test(NAME smoke COMMAND rtype_smoke_test)
