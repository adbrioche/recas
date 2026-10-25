#include "command_handler.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace recas {

// Helper Functions
namespace {

Value bad_arg_size(const char* command) {
  return Error(
    std::string("wrong number of arguments for '") + command
  );
}

Value handle_ping(const Array& array) {
  if (array.size() != 1) 
    return bad_arg_size("ping");

  return SimpleString("PONG");
}

Value handle_get(Store& store, const Array& array) {
  if (array.size() != 2) 
    return bad_arg_size("get");

  const auto* key = std::get_if<BulkString>(&array[1]);
  if (!key) 
    return Error("key must be a bulk string");

  auto result = store.get(*key);
  return result ? *result : Value(Null{});
}

Value handle_set(Store& store, const Array& array) {
  if (array.size() != 3) 
    return bad_arg_size("set");

  const auto* key = std::get_if<BulkString>(&array[1]);
  const auto* val = std::get_if<BulkString>(&array[2]);
  if (!key || !val) 
    return Error("key and value must be bulk strings");

  store.set(*key, *val);
  return SimpleString("OK");
}

Value handle_del(Store& store, const Array& array) {
  if (array.size() != 2) 
    return bad_arg_size("del");

  const auto* key = std::get_if<BulkString>(&array[1]);
  if (!key) 
    return Error("key must be a bulk string");

  return Integer{store.del(*key) ? 1 : 0};
}

}

Value handle_command(Store& store, const Value& value) {
  const auto* array = std::get_if<Array>(&value);
  if (!array || array->empty())
    return Error("command must be a non-empty RESP array");

  const auto* command_value = std::get_if<BulkString>(&array->front());
  if (!command_value)
    return Error("command name must be a bulk string");

  std::string command = *command_value;
  for (char& c : command)
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

  if (command == "PING") 
    return handle_ping(*array);

  if (command == "GET")  
    return handle_get(store, *array);

  if (command == "SET")
    return handle_set(store, *array);

  if (command == "DEL")  
    return handle_del(store, *array);

  return Error("unknown command");
}

}