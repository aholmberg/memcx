#ifndef REQUEST_H
#define REQUEST_H

#include <future>
#include <string>

#include <uv.h>

#include "memcx.h"

namespace memcx {

class Buffer;

namespace memcuv {

template<typename T>
void OnCloseHandle(uv_handle_t* handle) {
  T* t = reinterpret_cast<T*>(handle);
  delete t;
}

class UvConnection;

class UvRequest {

public:
  UvRequest(const std::string& command);
  virtual ~UvRequest();

  virtual size_t Ingest(Buffer& buffer) = 0;
  
  void Notify();
  
  void StartTimer(uv_loop_t* loop, uint64_t timeout_ms);

  const std::string& command() { return command_; }

  bool complete() { return complete_; }
  void set_complete(bool complete) { complete_ = complete; }

  UvConnection* connection() { return connection_; }
  void set_connection(UvConnection* connection) { connection_ = connection; }

  const std::string&  error_msg() { return error_msg_; }
  void set_error_msg(const std::string& msg) { error_msg_ = error_msg_.empty() ? msg : error_msg_; }
  bool IsError() { return !error_msg_.empty(); }

  bool notified() { return notified_; }

  const std::chrono::time_point<std::chrono::steady_clock>& start_time() { return start_time_; }
  void set_start_time(const std::chrono::time_point<std::chrono::steady_clock>& t) { start_time_ = t; }

  int64_t timeout() { return timeout_ms_; }
  void set_timeout(const int64_t timeout_ms) { timeout_ms_ = timeout_ms; }

private:
  virtual void DoNotify() = 0;
  void StopTimer();
  static void Timeout(uv_timer_t* handle, int status);

  std::string command_;
  bool complete_;
  UvConnection* connection_;
  std::string error_msg_;;
  bool notified_;
  std::chrono::time_point<std::chrono::steady_clock> start_time_;
  uint64_t timeout_ms_;
  uv_timer_t* timer_;
};

class SetRequest: public UvRequest {

public:
  SetRequest(const std::string& key, 
             const std::string& value, 
             const memcx::SetCallback& callback, 
             unsigned int flags = 0, 
             unsigned int exp_time = 0);
  virtual ~SetRequest() = default;

  virtual size_t Ingest(Buffer& buffer);

  static std::string CommandFromParams(const std::string& key, 
                                       const std::string& value, 
                                       unsigned int flags, 
                                       unsigned int exp_time);

private:
  virtual void DoNotify();

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

class GetRequest: public UvRequest {

enum ResponseState { PENDING_RESPONSE, HEADER_PARSED, VALUE_COMPLETE };

public:
  GetRequest(const std::string& key, const memcx::GetCallback& callback);
  virtual ~GetRequest() = default;

  virtual size_t Ingest(Buffer& buffer);

  static std::string CommandFromKey(const std::string& key);

private:
  virtual void DoNotify();

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
}
#endif
