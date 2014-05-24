#ifndef BUFFER_H
#define BUFFER_H

#include <string>

#include <uv.h>

namespace memcx {

class Buffer {

public:
  static const char* ENDL;
  static const size_t ENDL_LEN;

  Buffer(size_t initial_size = 64 * 1024);// this is fixed size in the current libuv
  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  virtual ~Buffer();

  uv_buf_t InitRead();

  void MarkNewBytesRead(size_t size);

  size_t ReadBytes(size_t count, std::string& dest);

  size_t ReadLine(std::string& line);

private:
  void ShiftRemaining();

  char* buffer_;
  char* read_cursor_;
  size_t capacity_;
  size_t current_size_;
};

}
#endif
