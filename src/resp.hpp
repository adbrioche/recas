#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>


namespace redis {

struct Value;

struct SimpleString {
    std::string value;
};

struct BulkString {
    std::string value;
};

struct Integer {
    int64_t value;
};

struct Error {
    std::string value;
};

struct Null {};

using Array = std::vector<Value>;

struct Value {
    std::variant<
        Null,
        Error,
        SimpleString,
        BulkString,
        Integer,
        Array
    > data;
};


class Parser {
public:
    void feed(const std::string& bytes);
    std::optional<Value> parse();

private:
    std::string buffer_;
    std::size_t position_ = 0;

    std::optional<Value> parse_value();
    std::optional<Value> parse_simple_string();
    std::optional<Value> parse_integer();
    std::optional<Value> parse_error();
    std::optional<Value> parse_bulk_string();
    std::optional<Value> parse_array();

    std::optional<std::string> read_line();
};

}