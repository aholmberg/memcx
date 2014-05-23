#ifndef UV_CONNECTION_H
#define UV_CONNECTION_H

#include <netinet/in.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <uv.h>
#include "request.h"
#include "buffer.h"
#include <memory>

namespace memcx {

typedef std::queue<Request*> RequestQueue;

namespace memcuv {


class UvConnection {

public:
  UvConnection(uv_loop_t *loop,
             const struct sockaddr_in &endpoint);
  virtual ~UvConnection();
//TODO: effective c++

  void WaitOnConnect();

  bool connected() { return connected_; }
  const std::string& connect_err() { return connect_err_; }
  void SubmitRequest(std::unique_ptr<Request> request);
/*
  void SetSync(const std::string &key, const std::string &value);
  void SetAsync(const std::string &key, const std::string &value, int callback);

  std::string GetSync(const std::string &key);
  void GetAsync(const std::string &key, int callback);
*/
  void Close();

private:
  void Connect();
  void NotifyConnect();
  void Reconnect();
  void ProcessData();

  uv_stream_t* socket_stream() { return reinterpret_cast<uv_stream_t*>(&socket_); }
  uv_handle_t* socket_handle() { return reinterpret_cast<uv_handle_t*>(&socket_); }

  static void WriteRequest(uv_async_t* handle, int status);
  //static void OnClose(uv_handle_t *stream);
  static void OnConnect(uv_connect_t *req, int status);
  static void OnWrite(uv_write_t *req, int status);
  static void OnRead(uv_stream_t* stream, ssize_t nread, uv_buf_t buf);
  static uv_buf_t GetBuffer(uv_handle_t* handle, size_t suggested_size);

  RequestQueue requests_;
  Buffer buffer_;
  uv_tcp_t socket_;
  bool connected_;
  uv_loop_t* loop_;
  struct sockaddr_in endpoint_;
  std::mutex wait_lock_;
  std::condition_variable connect_condition_;
  std::string connect_err_;
};

}
}
#endif
