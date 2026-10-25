#pragma once

namespace redis {

class FileDescriptor {
public:
  explicit FileDescriptor(int fd) noexcept;
  ~FileDescriptor();

  FileDescriptor(const FileDescriptor&) = delete;
  FileDescriptor& operator=(const FileDescriptor&) = delete;

  FileDescriptor(FileDescriptor&& other) noexcept;
  FileDescriptor& operator=(FileDescriptor&& other) noexcept;

  int get() const noexcept;

private:
  int fd_ = -1;
};

}