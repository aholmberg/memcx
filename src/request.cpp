#include "request.h"

#include <stdlib.h>

#include <exception>
using std::runtime_error;
#include <sstream>
using std::stringstream;

using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;
using std::string;

using namespace memcx;

#include "buffer.h"

Request::Request(const string& command):
  command_(command),
  complete_(false),
  data_(nullptr) {}

SetRequest::SetRequest(const string& key, 
                       const string& value, 
                       const SetCallback& callback, 
                       unsigned int flags, 
                       unsigned int exp_time):
  Request(CommandFromParams(key, value, flags, exp_time)),
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
      error_msg("memcached: ERROR");
      set_complete(true);
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

void SetRequest::Notify() {
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
  Request(CommandFromKey(key)),
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
        } else {
          error_msg(line);
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
        if (line == "END") {
          set_complete(true); 
        } else if (!line.empty()) {//endl following value bytes
          error_msg("unexpected response: " + line);
          set_complete(true);
        }
      }
      break;
    default://complete
      break;
    }
  } while(read_count > 0);
  return read_count_total;
}

void GetRequest::Notify() {
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
