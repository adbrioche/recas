
// handler should take in an array from resp, and we can use a biiiiiig switch case as a look-up table for how to handle each thing.

Command parse_command(const std::string& str) {
    if (str == "GET")  return Command::GET;
    if (str == "SET")  return Command::SET;
    if (str == "DEL")  return Command::DEL;
    if (str == "PING") return Command::PING;

    return Command::UNKNOWN;
}

bool handle(const Array& array) {
    const std::string& str = std::get<BulkString>(array[2].data).value;
    
    switch (parse_command(command)) {
        case Command::GET:
            // ...
            break;

        case Command::SET:
            // ...
            break;

        case Command::DEL:
            // ...
            break;

        case Command::PING:
            // ...
            break;

        case Command::UNKNOWN:
            // ...
            break;
    }
}


// if (auto* array = value ? std::get_if<Array>(&*value) : nullptr) {
//     do_something(*array);
// }