#ifndef MEMCACHE_UV_H
#define MEMCACHE_UV_H

#include <netinet/in.h>

#include <chrono>
#include <list>
#include <string>
#include <thread>

#include "uv_connection.h"

struct uv_loop_s;
typedef uv_loop_s uv_loop_t;

namespace memcx {
namespace memcuv {

typedef std::list<UvConnection> ConnectionList;
typedef std::list<UvConnection>::iterator ConnectionIter;

class MemcacheUv {

public:
  MemcacheUv(const std::string& host,
             const int port,
             const size_t pool_size);
  MemcacheUv(const MemcacheUv&) = delete;
  MemcacheUv& operator=(const MemcacheUv&) = delete;
  ~MemcacheUv();

  void SendRequest(std::unique_ptr<Request> req);
  void WaitConnect(const std::chrono::milliseconds& timeout);

private:
  void ConnectEvent(UvConnection* connection);
  ConnectionIter NextOpenConnection();
  void LoopThread();
  void NewMessageAsync();
  static void SendNewMessages(uv_async_t* handle, int status);
  static void Shutdown(uv_async_t* handle, int status);

  uv_loop_t* loop_;
  std::thread loop_thread_;

  //----
  std::mutex wait_lock_;
  std::condition_variable connect_condition_;
  //vvvvv
  RequestQueue new_requests_;
  ConnectionList connections_;
  ConnectionIter cursor_;
  size_t live_connections_;
  //^^^^^
  //----

  uv_async_t request_async_;
  uv_async_t shutdown_async_;
};

}
}
#endif
