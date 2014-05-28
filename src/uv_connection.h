#ifndef UV_CONNECTION_H
#define UV_CONNECTION_H

#include <netinet/in.h>

#include <memory>
#include <queue>

#include "buffer.h"
#include "request.h"

namespace memcx {

typedef std::queue<Request*> RequestQueue;

namespace memcuv {

class UvConnection {

public:

  typedef std::function<void(UvConnection*)> ConnectionCallback;

  UvConnection(uv_loop_t *loop,
               const struct sockaddr_in &endpoint,
               const ConnectionCallback& connect_callback);
  ~UvConnection();
  UvConnection(const UvConnection&) = delete;
  UvConnection& operator=(const UvConnection&) = delete;

  void Close(const std::string& message);

  void WriteRequest(std::unique_ptr<Request> request);

  bool connected() { return connected_; }

private:
  void Connect();
  void NotifyConnect();
  void ProcessData();
  void Reset(const char* context);

  std::string GetErrorMessage(const char* context);

  uv_stream_t* socket_stream() { return reinterpret_cast<uv_stream_t*>(socket_); }
  uv_handle_t* socket_handle() { return reinterpret_cast<uv_handle_t*>(socket_); }

  static void OnConnect(uv_connect_t *req, int status);
  static void Reconnect(uv_timer_t* handle, int status);
  static void WriteRequest(uv_async_t* handle, int status);
  static void OnWrite(uv_write_t *req, int status);
  static uv_buf_t GetBuffer(uv_handle_t* handle, size_t suggested_size);
  static void OnRead(uv_stream_t* stream, ssize_t nread, uv_buf_t buf);

  Buffer buffer_;
  ConnectionCallback connect_callback_;
  std::string connect_err_;
  bool connected_;
  struct sockaddr_in endpoint_;
  uv_loop_t* loop_;
  RequestQueue requests_;
  uv_tcp_t* socket_;
};

}
}
#endif
