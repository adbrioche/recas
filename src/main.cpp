#include <iostream>
#include <string>

#include "server.hpp"

int main(int argc, char* argv[]) {
  // Default Launch Options
  std::string ip = "127.0.0.1";
  int port = 6379;
  bool verbose = false;
  
  // Arguments
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];

    // Verbose
    if (arg == "--verbose" || arg == "-v")
      verbose = true;

    // IP configuration
    else if (arg == "--ip" || arg == "-ip") {
      if (++i >= argc) {
        std::cerr << "[ERROR] " << arg << " requires an IPv4 address.\n";
        return 1;
      }

      ip = argv[i];
    }
      
    // Port configuration
    else if (arg == "--port" || arg == "-p") {
      try {
        if (++i >= argc)
          throw std::invalid_argument("Invalid or missing port.");

        std::size_t charsRead = 0;
        const std::string text = argv[i];
        const int number = std::stoi(text, &charsRead);

        if (charsRead != text.size())
            throw std::invalid_argument("Invalid or missing port.");

        if (number < 1 || number > 65535)
            throw std::out_of_range("Port out of range (expected 1-65535).");
    
        port = number;
      } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
      }
    } 
    
    // Unknown argument
    else {
      std::cerr << "[ERROR] Unknown argument: \"" << arg << "\".\n";
      return 1;
    }
  }

  // Run server, print errors if they occur
  try {
    if (verbose) {
      std::cout << "Remote Dictionary Server\n"
                << "Listening address: " << ip << ":" << port << "\n"
                << "Initializing...\n";
    }
    
    recas::Server server(ip, port, verbose);
    server.run();
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] " << e.what() << '\n';
    return 1;
  }

  return 0;
}