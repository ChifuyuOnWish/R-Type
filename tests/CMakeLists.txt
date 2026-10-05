add_executable(rtype_smoke_test smoke_test.cpp)
target_link_libraries(rtype_smoke_test PRIVATE rtype::protocol)
add_test(NAME smoke COMMAND rtype_smoke_test)
