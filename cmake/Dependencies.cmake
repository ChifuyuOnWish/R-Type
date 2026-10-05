# All third-party dependencies go here, one CPMAddPackage per library.
# Never copy dependency sources into the repository.

# Standalone Asio (header-only, no Boost). Exposed as the asio::asio target.
CPMAddPackage(
    NAME asio
    GITHUB_REPOSITORY chriskohlhoff/asio
    GIT_TAG asio-1-30-2
    DOWNLOAD_ONLY YES
)

if(asio_ADDED)
    find_package(Threads REQUIRED)

    add_library(asio INTERFACE)
    target_include_directories(asio SYSTEM INTERFACE ${asio_SOURCE_DIR}/asio/include)
    target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
    target_link_libraries(asio INTERFACE Threads::Threads)

    if(WIN32)
        # Target Windows 10+ and link the Winsock libraries Asio needs.
        target_compile_definitions(asio INTERFACE _WIN32_WINNT=0x0A00)
        target_link_libraries(asio INTERFACE ws2_32 mswsock)
    endif()

    add_library(asio::asio ALIAS asio)
endif()
