#pragma once

#include "codec.hpp"
#include "store.hpp"

namespace redis {

Value handle_command(Store& store, const Value& value);

}