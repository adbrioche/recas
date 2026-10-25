#include <stdexcept>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <charconv>
#include <limits>

#include "codec.hpp"

#include <iostream>

namespace recas {

namespace {

constexpr std::size_t kMaxBulkLength = 1024 * 1024;
constexpr std::size_t kMaxArrayLength = 1024;
constexpr unsigned kMaxNestingDepth = 32;

static std::optional<Value> parse_value(
  std::string& buffer,
  std::size_t& position,
  unsigned depth
);

static std::optional<std::string> read_line(
  const std::string& buffer,
  std::size_t& position
) {
  auto end = buffer.find("\r\n", position);

  if (end == std::string::npos)
    return std::nullopt;

  auto line = buffer.substr(position, end - position);
  position = end + 2;

  return line;
}

static std::optional<Value> parse_simple_string(
  std::string& buffer,
  std::size_t& position
) {
  ++position;

  auto line = read_line(buffer, position);

  if (!line)
    return std::nullopt;

  return SimpleString{*line};
}

static std::optional<Value> parse_error(
  std::string& buffer,
  std::size_t& position
) {
  ++position;

  auto line = read_line(buffer, position);

  if (!line)
    return std::nullopt;

  return Error{*line};
}

static std::optional<Value> parse_integer(
  std::string& buffer,
  std::size_t& position
) {
  ++position;

  auto line = read_line(buffer, position);

  if (!line)
    return std::nullopt;

  std::int64_t value{};

  const auto [ptr, ec] = std::from_chars(
    line->data(),
    line->data() + line->size(),
    value
  );

  if (ec != std::errc{} || ptr != line->data() + line->size())
    return Error{"invalid RESP integer"};

  return Integer{value};
}

static std::optional<Value> parse_bulk_string(
  std::string& buffer,
  std::size_t& position
) {
  ++position;

  auto line = read_line(buffer, position);

  if (!line)
    return std::nullopt;

  std::int64_t length{};

  const auto [ptr, ec] = std::from_chars(
    line->data(),
    line->data() + line->size(),
    length
  );

  if (ec != std::errc{} || ptr != line->data() + line->size())
    return Error{"invalid bulk string length"};

  if (length == -1)
    return Null{};

  if (length < 0 || static_cast<std::uint64_t>(length) > kMaxBulkLength)
    return Error{"bulk string length out of range"};

  auto size = static_cast<std::size_t>(length);

  if (buffer.size() - position < size + 2)
    return std::nullopt;

  if (buffer.substr(position + size, 2) != "\r\n")
    return Error{"bulk string missing CRLF terminator"};

  std::string value = buffer.substr(position, size);

  position += size + 2;

  return BulkString{std::move(value)};
}

static std::optional<Value> parse_array(
  std::string& buffer,
  std::size_t& position,
  unsigned depth
) {
  ++position;

  auto line = read_line(buffer, position);

  if (!line)
    return std::nullopt;

  std::int64_t count{};

  const auto [ptr, ec] = std::from_chars(
    line->data(),
    line->data() + line->size(),
    count
  );

  if (ec != std::errc{} || ptr != line->data() + line->size())
    return Error{"invalid array length"};

  if (count == -1)
    return Null{};

  if (count < 0 || static_cast<std::uint64_t>(count) > kMaxArrayLength)
    return Error{"array length out of range"};

  Array values;
  values.reserve(static_cast<std::size_t>(count));

  for (std::int64_t i = 0; i < count; ++i) {
    auto value = parse_value(buffer, position, depth + 1);

    if (!value)
      return std::nullopt;

    values.push_back(std::move(*value));
  }

  return values;
}

static std::optional<Value> parse_value(
  std::string& buffer,
  std::size_t& position,
  unsigned depth
) {
  if (position >= buffer.size())
    return std::nullopt;

  if (depth > kMaxNestingDepth)
    return Error{"RESP nesting limit exceeded"};

  switch (buffer[position]) {
    case '+':
      return parse_simple_string(buffer, position);

    case '-':
      return parse_error(buffer, position);

    case ':':
      return parse_integer(buffer, position);

    case '$':
      return parse_bulk_string(buffer, position);

    case '*':
      return parse_array(buffer, position, depth);

    default: {
        ++position;

        auto line = read_line(buffer, position);

        if (!line)
            return std::nullopt;

        return Error{"invalid RESP type prefix"};
    }
  }
}

} // namespace

std::optional<Value> decode(std::string& buffer) {
  std::size_t position = 0;

  auto value = parse_value(buffer, position, 0);

  if (value)
    buffer.erase(0, position);

  return value;
}

std::string encode(const Value& value) {
  if (std::holds_alternative<Null>(value))
    return "$-1\r\n";

  if (std::holds_alternative<SimpleString>(value))
    return "+" + std::get<SimpleString>(value) + "\r\n";

  if (std::holds_alternative<Error>(value))
    return "-" + std::get<Error>(value) + "\r\n";

  if (std::holds_alternative<Integer>(value))
    return ":" + std::to_string(std::get<Integer>(value).value) + "\r\n";

  if (std::holds_alternative<BulkString>(value)) {
    auto& string = std::get<BulkString>(value);

    return "$" + std::to_string(string.size()) + "\r\n"
      + string + "\r\n";
  }

  if (std::holds_alternative<Array>(value)) {
    auto& array = std::get<Array>(value);

    std::string result = "*" + std::to_string(array.size()) + "\r\n";

    for (const auto& item : array)
      result += encode(item);

    return result;
  }

  return "-unknown Value type\r\n";
}

}