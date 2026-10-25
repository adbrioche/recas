#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <sys/epoll.h>

#include "client.hpp"
#include "file_descriptor.hpp"
#include "store.hpp"

namespace recas {

class Server {
public:
  Server(const std::string& ip, int port, bool verbose);
  void run();
  void shutdown();

private:
  std::string ip_;
  int port_;
  bool verbose_;

  FileDescriptor lfd_;
  FileDescriptor epfd_;
  std::unordered_map<int, Client> clients_;
  Store store_;

  FileDescriptor create_listen();
  FileDescriptor create_epoll();
  void add_to_epoll(int fd);

  void accept_clients();
  void handle_client(const epoll_event& event);

  void update_client(int fd, bool want_write);
  void close_client(int fd) noexcept;
  void log_info(const std::string& message) const;
};

}
