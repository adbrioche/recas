#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

int main() {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in saddr{};
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    saddr.sin_port = htons(6379);

    if (bind(sfd, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr)) == -1) {
        std::cerr << "bind() failed: " << strerror(errno) << '\n';
        close(sfd);
        return 1;
    }

    if (listen(sfd, 32) == -1) {
        std::cerr << "listen() failed\n";
        close(sfd);
        return 1;
    }

    std::cout << "Listening on 127.0.0.1:6379...\n";

    int cfd = accept(sfd, nullptr, nullptr);

    if (cfd == -1) {
        std::cerr << "accept() failed\n";
        close(sfd);
        return 1;
    }

    std::cout << "Client connected!\n";

    char buf[1024];

    ssize_t n = read(cfd, buf, sizeof(buf));

    if (n == -1) {
        std::cerr << "read() failed\n";
        close(cfd);
        close(sfd);
        return 1;
    }

    if (n == 5 && std::string_view(buf, n) == "PING\n") {
        const char response[] = "+PONG\r\n";

        if (write(cfd, response, sizeof(response) - 1) == -1) {
            std::cerr << "write() failed\n";
            close(cfd);
            close(sfd);
            return 1;
        }
    } else {
        std::cout.write(buf, n) << '\n';

        close(cfd);
        close(sfd);

        return 0;
    }
}