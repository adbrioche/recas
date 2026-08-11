#pragma once

#include <netinet/in.h>

#include "resp.hpp"

namespace redis {

class Server {
public:
    bool start();
    bool run();

private:
    int sfd_, cfd_;
    sockaddr_in saddr_;
    char buffer_[1024];
    Parser parser_;
};

}