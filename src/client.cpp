#include "client.hpp"

#include <utility>

namespace redis {

Client::Client(FileDescriptor fd)
  : fd_(std::move(fd)) {}

int Client::fd() const noexcept {
  return fd_.get();
}

std::string& Client::read_buffer() noexcept {
  return read_buffer_;
}

std::string& Client::write_buffer() noexcept {
  return write_buffer_;
}

bool Client::peer_closed() const noexcept {
  return peer_closed_;
}

void Client::mark_peer_closed() noexcept {
  peer_closed_ = true;
}

}