#include "file_descriptor.hpp"

#include <unistd.h>
#include <utility>

namespace recas {

FileDescriptor::FileDescriptor(int fd) noexcept
  : fd_(fd) {}

FileDescriptor::~FileDescriptor() {
  if (fd_ >= 0)
    ::close(fd_);
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept
  : fd_(std::exchange(other.fd_, -1)) {}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
  if (this != &other) {
    if (fd_ >= 0)
      ::close(fd_);

    fd_ = std::exchange(other.fd_, -1);
  }

  return *this;
}

int FileDescriptor::get() const noexcept {
  return fd_;
}

}
