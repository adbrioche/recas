#include "server.hpp"

#include "codec.hpp"
#include "command_handler.hpp"

#include <cerrno>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <system_error>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace recas {

namespace {
  class ShutdownException : public std::exception {
  public:
    const char* what() const noexcept override {
      return "Server shutdown requested";
    }
  };
}

Server::Server(const std::string& ip, int port, bool verbose)
  : ip_(ip),
    port_(port),
    verbose_(verbose),
    lfd_(create_listen()),
    epfd_(create_epoll()) {
  add_to_epoll(lfd_.get());
}

void Server::run() {
  log_info("Server running.");

  // Epoll event list.
  std::vector<epoll_event> events(64);

  // Event loop.
  while (true) {
    // Get event updates.
    const int count = ::epoll_wait(
      epfd_.get(),
      events.data(),
      static_cast<int>(events.size()),
      -1
    );

    // Special epoll case: either error or interrupt.
    if (count == -1) {
      // SIGINT received, interrupting epoll_wait.
      if (errno == EINTR)
        break;

      throw std::system_error(
        errno,
        std::generic_category(),
        "epoll_wait failed"
      );
    }

    // Listening descriptor means new connection, else existing client.
    for (int i = 0; i < count; ++i) {
      if (events[i].data.fd == lfd_.get())
        accept_clients();
      else
        handle_client(events[i]);
    }
  } 

  shutdown();
}

void Server::shutdown() {
  log_info("Server stopped.");
}

void Server::accept_clients() {
  // Accepting loop.
  while (true) {
    const int cfd = ::accept4(lfd_.get(), nullptr, nullptr, SOCK_NONBLOCK);

    if (cfd == -1) {
      // Interrupt received means a shutdown was requested.
      if (errno == EINTR)
        throw ShutdownException();

      // No more clients to accept.
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;

      throw std::system_error(
        errno,
        std::generic_category(),
        "Could not accept client."
      );
    }

    // Add to client list.
    clients_.emplace(cfd, Client(FileDescriptor(cfd)));

    // If epoll fails, remove the client before propagating the error.
    try {
      add_to_epoll(cfd);
    } catch (const std::system_error&) {
      clients_.erase(cfd);
      throw;
    }

    log_info("Accepted client.");
  }
}


void Server::handle_client(const epoll_event& event) {
  const int fd = event.data.fd;

  const bool has_error = event.events & (EPOLLERR | EPOLLHUP);
  const bool can_read = event.events & EPOLLIN;

  // Processing any data left over before closing a connection.
  if (has_error && !can_read) {
    close_client(fd);
    return;
  }

  Client& client = clients_.at(fd);

  // Processing incoming data.
  if (can_read) {
    char buffer[4096];

    while (true) {
      const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);

      // No data; connection explicitly closed on the other end.
      if (n == 0) {
        client.mark_peer_closed();
        break;
      }

      // Data to read 
      if (n > 0) {
        client.read_buffer().append(buffer, static_cast<std::size_t>(n));

        if (client.read_buffer().size() > 2 * 1024 * 1024) {
          client.write_buffer() 
            += encode(Error("request buffer limit exceeded"));
          update_client(fd, true);
          return;
        }

        continue;
      }

      // SIGINT means a shutdown was requested server-side. 
      if (errno == EINTR)
        throw ShutdownException();

      // No more data to go through, client handled successfully.
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;

      // Any other case not covered above: just close the connection.
      close_client(fd);
      return;
    }

    while (auto value = decode(client.read_buffer())) {
      if (std::holds_alternative<Error>(*value))
        client.write_buffer() += encode(*value);
      else
        client.write_buffer() += encode(handle_command(store_, *value));
    }
  }

  if (event.events & EPOLLOUT) {
    while (!client.write_buffer().empty()) {
      const ssize_t n = ::send(
        fd,
        client.write_buffer().data(),
        client.write_buffer().size(),
        MSG_NOSIGNAL
      );

      if (n > 0) {
        client.write_buffer().erase(0, static_cast<std::size_t>(n));
        continue;
      }

      // SIGINT means a shutdown was requested.
      if (n == -1 && errno == EINTR)
        throw ShutdownException();

      // Finished.
      if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        break;

      // Other errors.
      close_client(fd);
      return;
    }
  }

  if (client.peer_closed() && client.write_buffer().empty())
    close_client(fd);
  else
    update_client(fd, !client.write_buffer().empty());
}

FileDescriptor Server::create_listen() {
  FileDescriptor fd(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0));
  if (fd.get() < 0) {
    throw std::system_error(
      errno,
      std::generic_category(),
      "Failed to create listening socket."
    );
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<std::uint16_t>(port_));

  if (::inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr) != 1)
    throw std::invalid_argument("Invalid IPv4 address: \"" + ip_ + "\".");

  const int temp = 1;
  if (
    ::setsockopt(fd.get(), SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(temp)
  ) < 0) {
    throw std::system_error(
      errno,
      std::generic_category(),
      "Failed to set SO_REUSEADDR."
    );
  }

  if (
    ::bind(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0
  ) {
    throw std::system_error(
      errno, 
      std::generic_category(), 
      "Failed to bind listening socket."
    );
  }

  if (::listen(fd.get(), SOMAXCONN) < 0) {
    throw std::system_error(
      errno, 
      std::generic_category(), 
      "Failed to listen on socket."
    );
  }

  log_info("Listening on " + ip_ + ":" + std::to_string(port_) + ".");
  return fd;
}

FileDescriptor Server::create_epoll() {
  const int fd = ::epoll_create1(0);
  if (fd < 0) {
    throw std::system_error(
      errno,
      std::generic_category(),
      "Failed to create epoll instance."
    );
  }

  return FileDescriptor(fd);
}

void Server::add_to_epoll(int fd) {
  epoll_event event{};
  event.events = EPOLLIN;
  event.data.fd = fd;

  if (::epoll_ctl(epfd_.get(), EPOLL_CTL_ADD, fd, &event) < 0) {
    throw std::system_error(
      errno,
      std::generic_category(),
      "Failed to add descriptor to epoll."
    );
  }
}

void Server::update_client(int fd, bool want_write) {
  epoll_event event{};
  event.events = static_cast<std::uint32_t>(EPOLLIN)
    | (want_write ? static_cast<std::uint32_t>(EPOLLOUT) : 0U);
  event.data.fd = fd;

  if (::epoll_ctl(epfd_.get(), EPOLL_CTL_MOD, fd, &event) < 0) {
    if (errno != ENOENT && errno != EBADF)
      log_info(
        "Could not update epoll events for client " 
        + std::to_string(fd) + "."
      );
    close_client(fd);
  }
}

void Server::close_client(int fd) noexcept {
  ::epoll_ctl(epfd_.get(), EPOLL_CTL_DEL, fd, nullptr);
  clients_.erase(fd);
  log_info("Closed client connection.");
}

void Server::log_info(const std::string& message) const {
  if (verbose_)
    std::cout << "[INFO] " << message << "\n";
}

}