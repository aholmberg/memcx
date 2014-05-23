#ifndef MEMCACHE_UV_H
#define MEMCACHE_UV_H

#include <netinet/in.h>
#include <string>
#include <thread>
#include <list>

#include "uv_connection.h"

struct uv_loop_s;
typedef uv_loop_s uv_loop_t;

namespace memcx {
namespace memcuv {

typedef std::list<UvConnection> ConnectionList;
//TODO: define get/set callback types
class MemcacheUv {

public:
  MemcacheUv(const std::string& host,
             const int port);
  ~MemcacheUv();

  void SendSet(std::unique_ptr<SetRequest> request);
//  void SetAsync(const std::string &key, const std::string &value, int callback);

//  std::string GetSync(const std::string &key);
//  void GetAsync(const std::string &key, int callback);

private:
  void CreateConnection();
  void LoopThread();
  static void Shutdown(uv_async_t* handle, int status);

  uv_loop_t* loop_;
  std::thread loop_thread_;
  struct sockaddr_in endpoint_;
  ConnectionList connections_;
};

}
}
#endif
