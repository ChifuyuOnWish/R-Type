# Network client

The project now includes an event-driven UDP client wrapper for the authoritative server model used by R-Type. The implementation lives in the network layer and is validated by loopback and failure tests.

## Overview

`NetworkClient` is responsible for:

- opening a UDP connection to a server
- tracking the connection state machine
- sending reliable and unreliable messages
- receiving datagrams, validating headers, and decoding payloads
- emitting structured events for connection success, disconnection and errors

The public API is defined around a small event model:

- `Connected`
- `Disconnected`
- `MessageReceived`
- `ConnectionFailed`

The event variant is declared in `network/include/NetworkEvent.hpp`.

## Configuration

Connection defaults are centralized in `network/include/NetworkConfig.hpp`.
Those constants cover the default host, port, timeout values, and connection retry policy used by the client implementation.

Typical configuration values include:

- default server address and port
- maximum connect attempts
- disconnect timeout
- heartbeat cadence used during the connection lifecycle

## Lifecycle

A client follows this lifecycle:

1. Construct a `NetworkClient` with optional `ClientConfig`.
2. Call `connect(host, port, name)`.
3. Wait for `Connected` or `ConnectionFailed`.
4. Send messages through `send(...)` while in the connected state.
5. Call `disconnect()` when the session ends.

The client exposes its current status via `state()`, which reports whether it is disconnected, connecting, connected or in a failing state.

## Sending and receiving data

The client serializes outgoing packets with the same binary protocol used by the server. It supports:

- reliable messages for important state transitions
- unreliable messages for frequent inputs and periodic updates
- header validation and payload decoding before passing events to the caller

The network layer keeps the protocol parsing and transport concerns separate from the game logic, which makes the client easy to test against a loopback server.

## Failure handling

The client distinguishes between a clean disconnect and a connection failure. A failed attempt emits `ConnectionFailed`, including the reason, such as:

- timeout while waiting for server acceptance
- rejected connection
- invalid protocol version
- server already full
- disconnect during the handshake

This behavior is covered by the network client test suite.

## Tests

The repository includes loopback tests for the client in `tests/network_client_test.cpp`.
They validate:

- successful connection against a real local server
- failure when the server is unavailable or rejects the client
- timeout and disconnect handling during connection setup

These tests are registered in `tests/CMakeLists.txt` and run as part of the project test suite.
