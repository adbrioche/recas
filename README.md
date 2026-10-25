# Project Description

Recas is a learning project built to explore C++, Linux networking, TCP sockets, and the Redis Serialization Protocol (RESP); it is intended for educational purposes only and should not be relied upon for any practical or production use.

## Features

- RESP value encoding and decoding, including fragmented and pipelined input
- In-memory string key-value storage
- `PING`, `SET`, `GET`, and `DEL` commands
- Basic command and malformed-input error handling
- C++ unit tests for the protocol codec and command handling

## Requirements

- CMake 3.20 or newer
- A C++20-compatible compiler

## Build

From the project root:

```sh
cmake -S . -B build
cmake --build build
```

## Run

To start the server with the default settings (`127.0.0.1:6379`):

```sh
./build/recas-server
```

Alternatively, use the helper script, which builds the project before starting the server:

```sh
./build.sh
```

Pass server options after the executable or script, for example:

```sh
./build/recas-server --ip 127.0.0.1 --port 6380 --verbose
# or
./build.sh --ip 127.0.0.1 --port 6380 --verbose
```

## Tests

Build and run the unit tests with CTest:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite covers RESP round trips, incomplete and pipelined input, malformed values, and command behaviour.