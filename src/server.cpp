#include <iostream>
#include <cstring>
#include <cerrno>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server.hpp"


namespace redis {

Server::Server() : sfd_(-1), cfd_(-1), saddr_{}, buffer_{}, parser_{} { }

// ~Server() {
//     if (cfd_ != -1) close(cfd_);
//     if (sfd_ != -1) close(sfd_);
// }

bool Server::start() {
    // Socket
    if ((sfd_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr << "socket() failed: " << strerror(errno) << '\n';
        return false;
    } else {
        saddr_.sin_family = AF_INET;
        saddr_.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        saddr_.sin_port = htons(6379);
    }

    // Bind
    if (bind(sfd_, reinterpret_cast<sockaddr*>(&saddr_), sizeof(saddr_)) == -1) {
        std::cerr << "bind() failed: " << strerror(errno) << '\n';
        close(sfd_);
        return false;
    }

    // Listen
    if (listen(sfd_, 32) == -1) {
        std::cerr << "listen() failed: " << strerror(errno) << '\n';
        close(sfd_);
        return false;
    } else {
        std::cout << "Listening on 127.0.0.1:6379...\n";
    }
}

bool Server::run() {
    // Accept
    if ((cfd_ = accept(sfd_, nullptr, nullptr)) == -1) {
        std::cerr << "accept() failed: " << strerror(errno) << '\n';
        close(sfd_);
        return false;
    } else {
        std::cout << "Client connected!\n";
    }

    // Loop
    while (true) {
        ssize_t n = read(cfd_, buffer_, sizeof(buffer_));

        if (n == -1) {
            std::cerr << "read() failed: " << strerror(errno) << '\n';
            return false;
        } else if (n == 0) {
            return true;
        } else {
            parser_.feed(buffer_);
            auto result = parser_.parse();

            if (!result) {
                continue;
            }
        }
    }
}

} 