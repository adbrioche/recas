#include "codec.hpp"
#include "command_handler.hpp"
#include "store.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition)
    throw std::runtime_error(message);
}

recas::Value decode_one(std::string& input, const std::string& description) {
  auto value = recas::decode(input);
  require(value.has_value(), description + ": expected a complete RESP value");
  return std::move(*value);
}

void test_codec() {
  using namespace recas;

  const Value values[] = {
    SimpleString("OK"),
    Error("ERR example"),
    Integer{42},
    BulkString("hello"),
    Null{},
    Array{BulkString("PING"), BulkString("hello"), Integer{7}}
  };

  for (const auto& value : values) {
    const std::string encoded = encode(value);
    std::string input = encoded;
    const Value decoded = decode_one(input, "RESP round trip");
    require(input.empty(), "decode should remove the consumed bytes");
    require(encode(decoded) == encoded, "encoded value should survive a round trip");
  }

  std::string partial = "$5\r\nhe";
  require(!decode(partial), "incomplete bulk string should not decode");
  require(partial == "$5\r\nhe", "incomplete input should be preserved");
  partial += "llo\r\n";
  const Value completed = decode_one(partial, "fragmented bulk string");
  require(std::get<BulkString>(completed) == "hello", "fragmented bulk string should decode");
  require(partial.empty(), "completed fragmented input should be consumed");

  std::string null_bulk = "$-1\r\n";
  require(std::holds_alternative<Null>(decode_one(null_bulk, "null bulk string")),
    "$-1 should decode as null");

  std::string null_array = "*-1\r\n";
  require(std::holds_alternative<Null>(decode_one(null_array, "null array")),
    "*-1 should decode as null");

  std::string pipelined = "+ONE\r\n:2\r\n$3\r\nEND\r\n";
  require(std::get<SimpleString>(decode_one(pipelined, "first pipelined value")) == "ONE",
    "first pipelined value should decode correctly");
  require(std::get<Integer>(decode_one(pipelined, "second pipelined value")).value == 2,
    "second pipelined value should decode correctly");
  require(std::get<BulkString>(decode_one(pipelined, "third pipelined value")) == "END",
    "third pipelined value should decode correctly");
  require(pipelined.empty(), "all pipelined values should be consumed");

  const std::string malformed_inputs[] = {
  "?invalid\r\n", // Unknown RESP type prefix.
  ":abc\r\n",   // Invalid integer.
  "$-2\r\n",  // Invalid negative bulk length.
  "*-2\r\n",  // Invalid negative array length.
  "$3\r\nabcXX",  // Missing CRLF after bulk data.
  "*x\r\n"  // Invalid array length.
  };

  for (const auto& malformed : malformed_inputs) {
    std::string input = malformed;
    bool rejected = false;
    try {
      auto value = decode(input);
      rejected = value && std::holds_alternative<Error>(*value);
    } catch (const std::exception&) {
      rejected = true;
    }
    require(rejected, "malformed RESP input should be reported as an error");
  }
}

void test_commands() {
  using namespace recas;
  Store store;

  auto result = handle_command(store, Array{BulkString("PING")});
  require(std::holds_alternative<SimpleString>(result) &&
    std::get<SimpleString>(result) == "PONG",
    "PING should return PONG");

  result = handle_command(store, Array{BulkString("ping")});
  require(std::holds_alternative<SimpleString>(result) &&
    std::get<SimpleString>(result) == "PONG",
    "command names should be case-insensitive");

  result = handle_command(store, Array{BulkString("SET"), BulkString("key"), BulkString("value")});
  require(std::holds_alternative<SimpleString>(result) &&
    std::get<SimpleString>(result) == "OK",
    "SET should return OK");

  result = handle_command(store, Array{BulkString("GET"), BulkString("key")});
  require(std::holds_alternative<BulkString>(result) &&
    std::get<BulkString>(result) == "value",
    "GET should return the stored value");

  result = handle_command(store, Array{BulkString("DEL"), BulkString("key")});
  require(std::holds_alternative<Integer>(result) && std::get<Integer>(result).value == 1,
    "DEL should return 1 when it deletes a key");

  result = handle_command(store, Array{BulkString("DEL"), BulkString("key")});
  require(std::holds_alternative<Integer>(result) && std::get<Integer>(result).value == 0,
    "DEL should return 0 for a missing key");

  result = handle_command(store, Array{BulkString("GET"), BulkString("missing")});
  require(std::holds_alternative<Null>(result), "GET should return null for a missing key");

  const Value invalid_commands[] = {
    Array{},
    SimpleString("PING"),
    Array{Integer{1}},
    Array{BulkString("PING"), BulkString("extra")},
    Array{BulkString("GET")},
    Array{BulkString("SET"), BulkString("key")},
    Array{BulkString("DEL")},
    Array{BulkString("NO_SUCH_COMMAND")},
    Array{BulkString("GET"), Integer{1}},
    Array{BulkString("SET"), BulkString("key"), Integer{1}}
  };

  for (const auto& command : invalid_commands) {
    result = handle_command(store, command);
    require(std::holds_alternative<Error>(result),
      "invalid command, argument count, or argument type should return an error");
  }
}

}

int main(int argc, char* argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: recas-tests <codec|commands>\n";
      return EXIT_FAILURE;
    }

    const std::string test = argv[1];

    if (test == "codec") {
      test_codec();
    } else if (test == "commands") {
      test_commands();
    } else {
      std::cerr << "Unknown test: " << test << '\n';
      return EXIT_FAILURE;
    }

    std::cout << "All tests passed\n";
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "Test failure: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}