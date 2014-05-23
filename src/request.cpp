#include "request.h"
#include "buffer.h"
//TODO: fix include and using organization
#include <exception>
#include <sstream>
using std::stringstream;
using std::runtime_error;
using std::string;
using std::unique_lock;
using std::mutex;

using std::placeholders::_1;
using namespace memcx;

Request::Request(const string& command):
command_(command),
complete_(false) {}
Request::~Request() {}

SetRequest::SetRequest(const std::string& key, const std::string& value, SetCallback callback, unsigned int flags, unsigned int exp_time):
Request(CommandFromParams(key, value, flags, exp_time)),
callback_(callback) {}

SetRequest::~SetRequest() {}

string SetRequest::CommandFromParams(const string& key, const string& value, unsigned int flags, unsigned int exp_time) {
  stringstream ss;
  ss << "set " << key << " " << flags << " " << exp_time << " " << value.size() << "\r\n";
  ss << value << "\r\n";
  return ss.str();
}

void SetRequest::Notify() {
  callback_(error_msg());
}

size_t SetRequest::Ingest(Buffer& buffer) {
  size_t read_count_total = 0;
  size_t read_count = 0;
  string line;
  while ((read_count = buffer.TryReadLine(line)) > 0) {
    read_count_total += read_count;
    if (line == "STORED") {
      complete(true);
      break;
    } else if (line == "ERROR") {
      error_msg("memcached: ERROR");
      complete(true);
      break;
    } else if (line.empty()) {
  // empty lines are eaten, as they are errors from previous commands
  // (e.g.: send a line with set *too-long key* with flag, exp, bytes)
      
    } else {
      error_msg(line);
    }
  }
  return read_count_total;
}

SetRequestAsync::SetRequestAsync(const std::string& key, const std::string& value, unsigned int flags, unsigned int exp_time):
SetRequest(key, value, std::bind(&SetRequestAsync::SetValue, this, _1), flags, exp_time) {}

void SetRequestAsync::SetValue(string error) {
  if (error.empty()) {
    promise_.set_value();
  } else {
    promise_.set_exception(make_exception_ptr(runtime_error(error)));
  }
}

GetRequest::GetRequest(const string& key):
Request(CommandFromKey(key)) {}

string GetRequest::CommandFromKey(const string& key) {
  return "get " + key + "\r\n";
}

size_t GetRequest::Ingest(Buffer& buffer) {
  return 0;
}
