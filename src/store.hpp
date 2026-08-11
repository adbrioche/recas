#pragma once

#include <optional>
#include <string>
#include <unordered_map>

class Store {
public:
    std::optional<std::string> get(const std::string& key) const;
    void set(const std::string& key, const std::string& value);
    bool contains(const std::string& key) const;
    bool erase(const std::string& key);

private:
    std::unordered_map<std::string, std::string> data_;
};