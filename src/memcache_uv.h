#ifndef MEMCACHE_UV_H
#define MEMCACHE_UV_H

#include <netinet/in.h>
#include <string>
#include <thread>
#include <list>
#include <chrono>

#include "uv_connection.h"

struct uv_loop_s;
typedef uv_loop_s uv_loop_t;

namespace memcx {
namespace memcuv {

typedef std::list<UvConnection> ConnectionList;
typedef std::list<UvConnection>::iterator ConnectionIter;
//TODO: define get/set callback types
class MemcacheUv {

public:
  MemcacheUv(const std::string& host,
             const int port,
             const size_t pool_size);
  ~MemcacheUv();

  void SendRequestSync(std::unique_ptr<Request> request, const std::chrono::milliseconds& timeout);
  void SendRequestAsync(std::unique_ptr<Request> request);
  void WaitConnect(const std::chrono::milliseconds& timeout);

private:
  ConnectionIter NextOpenConnection();
  void ConnectEvent(UvConnection* connection);
  void LoopThread();
  static void Shutdown(uv_async_t* handle, int status);
  static void OnCloseAsync(uv_handle_t* async);

  uv_loop_t* loop_;
  std::thread loop_thread_;
  struct sockaddr_in endpoint_;
  ConnectionList connections_;
  std::mutex wait_lock_;
  std::condition_variable connect_condition_;
  size_t live_connections_;
  ConnectionIter cursor_;
};

}
}
#endif
