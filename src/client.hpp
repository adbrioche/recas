#pragma once

#include <string>

#include "file_descriptor.hpp"

namespace redis {

class Client {
public:
  explicit Client(FileDescriptor fd);

  int fd() const noexcept;
  std::string& read_buffer() noexcept;
  std::string& write_buffer() noexcept;
  bool peer_closed() const noexcept;
  void mark_peer_closed() noexcept;

private:
  FileDescriptor fd_;
  std::string read_buffer_;
  std::string write_buffer_;
  bool peer_closed_ = false;
};

}
