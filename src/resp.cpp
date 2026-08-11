#include "resp.hpp"

#include <charconv>
#include <stdexcept>


namespace redis {

void Parser::feed(const char* data, std::size_t length); {
    buffer_.append(bytes);
}

std::optional<Value> Parser::parse() {
    position_ = 0;

    auto result = parse_value();

    if (result) {
        buffer_.erase(0, position_);
    }

    return result;
}

std::optional<Value> Parser::parse_value() {
    if (position_ >= buffer_.size()) {
        return std::nullopt;
    }

    switch (buffer_[position_]) {
        case '+':
            return parse_simple_string();
        case '-':
            return parse_error();
        case ':':
            return parse_integer();
        case '$':
            return parse_bulk_string();
        case '*':
            return parse_array();
        default:
            throw std::runtime_error("invalid formatting");
    }
}

std::optional<std::string> Parser::read_line() {
    auto end = buffer_.find("\r\n", position_);

    if (end == std::string::npos) {
        return std::nullopt;
    }

    std::string line = buffer_.substr(
        position_,
        end - position_
    );

    position_ = end + 2;

    return line;
}

std::optional<Value> Parser::parse_simple_string() {
    ++position_;

    auto line = read_line();

    if (!line) {
        return std::nullopt;
    }

    return Value{SimpleString{*line}};
}

std::optional<Value> Parser::parse_error() {
    ++position_;

    auto line = read_line();

    if (!line) {
        return std::nullopt;
    }

    return Value{Error{*line}};
}

std::optional<Value> Parser::parse_integer() {
    ++position_; // ':'

    auto line = read_line();

    if (!line) {
        return std::nullopt;
    }

    int64_t value;

    auto [ptr, ec] = std::from_chars(
        line->data(),
        line->data() + line->size(),
        value
    );

    if (ec != std::errc{} || ptr != line->data() + line->size()) {
        throw std::runtime_error("Invalid RESP integer");
    }

    return Value{Integer{value}};
}

std::optional<Value> Parser::parse_bulk_string() {
    ++position_; // '$'

    auto line = read_line();

    if (!line) {
        return std::nullopt;
    }

    std::size_t length = std::stoull(*line);

    // We need `length` bytes + "\r\n".
    if (buffer_.size() - position_ < length + 2) {
        return std::nullopt;
    }

    std::string value = buffer_.substr(position_, length);

    position_ += length;

    if (buffer_.substr(position_, 2) != "\r\n") {
        throw std::runtime_error("Invalid bulk string");
    }

    position_ += 2;

    return Value{BulkString{std::move(value)}};
}

std::optional<Value> Parser::parse_array() {
    ++position_; // '*'

    auto line = read_line();

    if (!line) {
        return std::nullopt;
    }

    std::size_t count = std::stoull(*line);

    Array values;
    values.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        auto value = parse_value();

        if (!value) {
            return std::nullopt;
        }

        values.push_back(std::move(*value));
    }

    return Value{std::move(values)};
}

}