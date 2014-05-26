#include "buffer.h"

#include <algorithm>
using std::copy;
using std::min;

using std::string;

using namespace memcx;

const char* Buffer::ENDL = "\r\n";
const size_t Buffer::ENDL_LEN = strlen(ENDL);

Buffer::Buffer(size_t size):
  buffer_(new char[size]),
  read_cursor_(buffer_),
  capacity_(size),
  current_size_(0) {}

Buffer::~Buffer() {
  delete[] buffer_;
}

uv_buf_t Buffer::InitRead() {
  ShiftRemaining();
  char* free_start = buffer_ + current_size_;
  unsigned int len = capacity_ - current_size_ - 1;
  return uv_buf_init(free_start, len);
}

void Buffer::MarkNewBytesRead(size_t size) {
  current_size_ += size;
  buffer_[current_size_] = '\0';
}

size_t Buffer::ReadBytes(size_t count, string& dest) {
  size_t bytes_available = buffer_ + current_size_ - read_cursor_;
  size_t chars_read = min(count, bytes_available);
  dest.append(read_cursor_, chars_read);
  read_cursor_ += chars_read;
  return chars_read;
}

size_t Buffer::ReadLine(string& line) {
  size_t chars_read = 0;
  line.clear();
  char* end = strstr(read_cursor_, ENDL);
  if (end != nullptr) {
    size_t span = end - read_cursor_;
    line.append(read_cursor_, span);
    chars_read = span + ENDL_LEN;
    read_cursor_ += chars_read;
  }
  return chars_read;
}

void Buffer::ShiftRemaining() {
  char* new_end = copy(read_cursor_, buffer_ + current_size_, buffer_);
  current_size_ = new_end - buffer_;
  read_cursor_ = buffer_;
}
