#pragma once

#include "codec.hpp"
#include "store.hpp"

namespace recas {

Value handle_command(Store& store, const Value& value);

}