#include <optional>
#include <string>
#include <unordered_map>

#include "codec.hpp"
#include "store.hpp"

namespace recas {

std::optional<Value> Store::get(const std::string& key) const {
  if (auto it = data_.find(key); it != data_.end())
    return it->second;
  else
    return std::nullopt;
}

void Store::set(const std::string& key, const Value& value) {
  data_[key] = value;
}

bool Store::del(const std::string& key) {
  return data_.erase(key) > 0;
}

}