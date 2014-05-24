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

  void SendSet(std::unique_ptr<SetRequest> request, const std::chrono::milliseconds& timeout);
//  void SetAsync(const std::string &key, const std::string &value, int callback);

//  std::string GetSync(const std::string &key);
//  void GetAsync(const std::string &key, int callback);
//  void WaitConnect();

private:
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
