cmake_minimum_required(VERSION 3.21)

project(rtype
    VERSION 0.1.0
    DESCRIPTION "Networked R-Type clone with a custom game engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
foreach(config IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} ${CMAKE_SOURCE_DIR})
endforeach()

option(RTYPE_BUILD_TESTS "Build the test suite" ON)

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    find_package(Git QUIET)
    if(GIT_FOUND)
        file(GLOB RTYPE_HOOKS "${CMAKE_SOURCE_DIR}/.githooks/*")
        file(CHMOD ${RTYPE_HOOKS}
            FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} config core.hooksPath .githooks
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()
endif()

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)
include(cmake/CompilerWarnings.cmake)

add_subdirectory(server)
add_subdirectory(client)

if(RTYPE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
