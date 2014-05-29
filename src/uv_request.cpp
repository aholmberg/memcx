#include "uv_request.h"

#include <stdlib.h>

#include <exception>
using std::runtime_error;
#include <sstream>
using std::stringstream;

using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;
using std::string;

using namespace memcx::memcuv;

#include "buffer.h"

UvRequest::UvRequest(const string& command):
  command_(command),
  complete_(false),
  connection_(nullptr),
  notified_(false),
  timeout_ms_(0),
  timer_(nullptr) {}

UvRequest::~UvRequest() {
  StopTimer();
}

void UvRequest::Notify() {
  if (!notified_) {
    DoNotify();
    notified_ = true; 
  }
  StopTimer();
}

void UvRequest::StartTimer(uv_loop_t* loop, uint64_t timeout_ms) {
  timer_ = new uv_timer_t();
  uv_timer_init(loop, timer_);
  timer_->data = this;
  uv_timer_start(timer_, Timeout, timeout_ms, 0);
}

void UvRequest::Timeout(uv_timer_t* handle, int status) {
  UvRequest* ths = static_cast<UvRequest*>(handle->data);
  ths->set_error_msg("timeout");
  ths->Notify();
}

void UvRequest::StopTimer() {
  if (timer_ != nullptr) {
    uv_timer_stop(timer_);
    uv_close(reinterpret_cast<uv_handle_t*>(timer_), OnCloseHandle<uv_timer_t>);
    timer_ = nullptr;
  }
}

SetRequest::SetRequest(const string& key, 
                       const string& value, 
                       const SetCallback& callback, 
                       unsigned int flags, 
                       unsigned int exp_time):
  UvRequest(CommandFromParams(key, value, flags, exp_time)),
  callback_(callback) {}

size_t SetRequest::Ingest(Buffer& buffer) {
  size_t read_count_total = 0;
  size_t read_count = 0;
  string line;
  while ((read_count = buffer.ReadLine(line)) > 0) {
    read_count_total += read_count;
    if (line == "STORED") {
      set_complete(true);
      break;
    } else if (line == "ERROR") {
      set_error_msg("memcached: ERROR");
      set_complete(true);
      break;
    } else if (line.empty()) {
  // empty lines are eaten, as they are errors from previous commands
  // (e.g.: send a line with set *too-long key* with flag, exp, bytes)
    } else {
      set_error_msg(line);
    }
  }
  return read_count_total;
}

void SetRequest::DoNotify() {
  callback_(error_msg());
}

string SetRequest::CommandFromParams(const string& key, 
                                     const string& value, 
                                     unsigned int flags, 
                                     unsigned int exp_time) {
  stringstream ss;
  ss << "set " << key << " " << flags << " " << exp_time << " " << value.size() << "\r\n";
  ss << value << "\r\n";
  return ss.str();
}

SetRequestAsync::SetRequestAsync(const string& key, 
                                 const string& value, 
                                 unsigned int flags, 
                                 unsigned int exp_time):
  SetRequest(key, value, bind(&SetRequestAsync::SetResponse, this, _1), flags, exp_time) {}

void SetRequestAsync::SetResponse(const string& error) {
  if (error.empty()) {
    promise_.set_value();
  } else {
    promise_.set_exception(make_exception_ptr(runtime_error(error)));
  }
}

GetRequest::GetRequest(const string& key, const GetCallback& callback):
  UvRequest(CommandFromKey(key)),
  callback_(callback),
  expected_size_(0),
  flags_(0),
  response_state_(PENDING_RESPONSE) {}

size_t GetRequest::Ingest(Buffer& buffer) {
  size_t read_count_total = 0;
  size_t read_count = 0;
  do {
    read_count = 0;
    string line;
    switch(response_state_) {
    case PENDING_RESPONSE:
      if ((read_count = buffer.ReadLine(line)) > 0) {
        read_count_total += read_count;
        if (line.find("VALUE") == 0) {
          ParseHeader(line);
          response_state_ = HEADER_PARSED;
        } else if (line == "END") {
          set_complete(true);
        } else {
          set_error_msg("memcached: " + line);
          set_complete(true);
        }
      }
      break;

    case HEADER_PARSED:
      read_count = buffer.ReadBytes(expected_size_-value_.size() , value_);
      read_count_total += read_count;
      if (value_.size() == expected_size_) {
        response_state_ = VALUE_COMPLETE;
      }
      break;

    case VALUE_COMPLETE:
      if ((read_count = buffer.ReadLine(line)) > 0) {
        read_count_total += read_count;
        if (line.empty()) {
          // empty line is endl following value bytes
        } else if (line == "END") {
          set_complete(true); 
        } else {
          set_error_msg("unexpected response: " + line);
          set_complete(true);
        }
      }
      break;
    default://complete
      break;
    }
  } while(read_count > 0 && !complete());
  return read_count_total;
}

void GetRequest::DoNotify() {
  callback_(value_, error_msg());
}

string GetRequest::CommandFromKey(const string& key) {
  return "get " + key + "\r\n";
}

void GetRequest::ParseHeader(const string& header) {
  string::size_type mark = header.rfind(' ');
  if (mark != string::npos) {
    expected_size_ = strtoul(header.c_str()+mark+1, nullptr, 10);
  }
  mark = header.rfind(' ', mark-1);
  if (mark != string::npos) {
    flags_ = strtoul(header.c_str()+mark+1, nullptr, 10);
  }
}

GetRequestAsync::GetRequestAsync(const string& key):
  GetRequest(key, bind(&GetRequestAsync::SetResponse, this, _1, _2)) {}

void GetRequestAsync::SetResponse(const string& value, const string& error) {
  if (error.empty()) {
    promise_.set_value(value);
  } else {
    promise_.set_exception(make_exception_ptr(runtime_error(error)));
  }
}
