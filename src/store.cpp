#include "store.hpp"

std::optional<std::string> Store::get(const std::string& key) const {
    if (auto it = data_.find(key); it != data_.end()) {
        return it->second;
    } else {
        return std::nullopt;
    }
}

void Store::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

bool Store::contains(const std::string& key) const {
    return data_.contains(key);
}

bool Store::erase(const std::string& key) {
    return data_.erase(key) > 0;
}