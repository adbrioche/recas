#include <iostream>

#include "server.hpp"


int main() {
    Server server;
    
    server.start();
    server.run();

    std::cout << "Shutdown complete.\n";

    return 0;
}





/*
This is server-side. When we start up our program, we want one acceptor thread which assigns a service thread to each client connection.
Each service thread should contain it's own read, write, and parsing logic.


*/









// int main() {
//     int sfd = socket(AF_INET, SOCK_STREAM, 0);

//     sockaddr_in saddr{};
//     saddr.sin_family = AF_INET;
//     saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
//     saddr.sin_port = htons(6379);

//     if (bind(sfd, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr)) == -1) {
//         std::cerr << "bind() failed: " << strerror(errno) << '\n';
//         close(sfd);
//         return 1;
//     }

//     if (listen(sfd, 32) == -1) {
//         std::cerr << "listen() failed\n";
//         close(sfd);
//         return 1;
//     }

//     std::cout << "Listening on 127.0.0.1:6379...\n";

//     int cfd = accept(sfd, nullptr, nullptr);

//     if (cfd == -1) {
//         std::cerr << "accept() failed\n";
//         close(sfd);
//         return 1;
//     }

//     std::cout << "Client connected!\n";

//     char buf[1024];

//     ssize_t n = read(cfd, buf, sizeof(buf));

//     if (n == -1) {
//         std::cerr << "read() failed\n";
//         close(cfd);
//         close(sfd);
//         return 1;
//     }

//     if (n == 14 && std::string_view(buf, n) == "*1\r\n$4\r\nPING\r\n") {
//         const char response[] = "+PONG\r\n";

//         if (write(cfd, response, sizeof(response) - 1) == -1) {
//             std::cerr << "write() failed\n";
//             close(cfd);
//             close(sfd);
//             return 1;
//         }
//     } else {
//         std::cout.write(buf, n) << '\n';

//         close(cfd);
//         close(sfd);

//         return 0;
//     }
// }