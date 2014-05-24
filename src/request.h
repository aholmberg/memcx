#ifndef REQUEST_H
#define REQUEST_H

#include <future>
#include <string>

#include <uv.h>

#include "memcx.h"

namespace memcx {

class Buffer;

class Request {

public:
  Request(const std::string& command);
  virtual ~Request() = default;

  virtual size_t Ingest(Buffer& buffer) = 0;
  
  virtual void Notify() = 0;

  const std::string& command() { return command_; }

  bool complete() { return complete_; }
  void set_complete(bool complete) { complete_ = complete; }

  void* data() { return data_; }
  void set_data(void* data) { data_ = data; }

  const std::string&  error_msg() { return error_msg_; }
  void error_msg(const std::string& msg) { error_msg_ = error_msg_.empty() ? msg : error_msg_; }
  bool IsError() { return !error_msg_.empty(); }

private:
  std::string command_;
  bool complete_;
  void* data_;
  std::string error_msg_;;
};

class SetRequest: public Request {

public:
  SetRequest(const std::string& key, 
             const std::string& value, 
             const memcx::SetCallback& callback, 
             unsigned int flags = 0, 
             unsigned int exp_time = 0);
  virtual ~SetRequest() = default;

  virtual size_t Ingest(Buffer& buffer);

  virtual void Notify();

  static std::string CommandFromParams(const std::string& key, 
                                       const std::string& value, 
                                       unsigned int flags, 
                                       unsigned int exp_time);

private:
  memcx::SetCallback callback_;
};

class SetRequestAsync: public SetRequest {
public:
  SetRequestAsync(const std::string& key, 
                 const std::string& value, 
                 unsigned int flags = 0, 
                 unsigned int exp_time = 0);
  virtual ~SetRequestAsync() = default;

  std::future<void> GetFuture() { return promise_.get_future(); }

  void SetResponse(const std::string& error);

private:
  std::promise<void> promise_;
};

class GetRequest: public Request {

enum ResponseState { PENDING_RESPONSE, HEADER_PARSED, VALUE_COMPLETE };

public:
  GetRequest(const std::string& key, const memcx::GetCallback& callback);
  virtual ~GetRequest() = default;

  virtual size_t Ingest(Buffer& buffer);

  virtual void Notify();

  static std::string CommandFromKey(const std::string& key);

private:
  void ParseHeader(const std::string& header);

  memcx::GetCallback callback_;
  size_t expected_size_;
  unsigned long int flags_;
  ResponseState response_state_;
  std::string value_;
};

class GetRequestAsync: public GetRequest {
public:
  GetRequestAsync(const std::string& key);
  virtual ~GetRequestAsync() = default;

  std::future<std::string> GetFuture() { return promise_.get_future(); }

  void SetResponse(const std::string& value, const std::string& error);

private:
  std::promise<std::string> promise_;
};

}
#endif
