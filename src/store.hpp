#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "codec.hpp"

namespace redis {

class Store {
public:
    std::optional<Value> get(const std::string& key) const;
    void set(const std::string& key, const Value& value);
    bool del(const std::string& key);

private:
    std::unordered_map<std::string, Value> data_;
};

}