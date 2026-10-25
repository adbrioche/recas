#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace recas {

struct SimpleString : std::string {
  using std::string::string;
  SimpleString(std::string s) : std::string(std::move(s)) {}
};

struct BulkString : std::string {
  using std::string::string;
  BulkString(std::string s) : std::string(std::move(s)) {}
};

struct Error : std::string {
  using std::string::string;
  Error(std::string s) : std::string(std::move(s)) {}
};

struct Integer {
  std::int64_t value;
};

struct Null {};

struct Value;

using Array = std::vector<Value>;

struct Value : std::variant<
  Null,
  Error,
  SimpleString,
  BulkString,
  Integer,
  Array
> {
  using variant::variant;
};

std::optional<Value> decode(std::string& buffer);
std::string encode(const Value& value);

}