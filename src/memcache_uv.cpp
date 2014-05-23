#include "memcache_uv.h"
#include <uv.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <memory>
using std::unique_ptr;
/*
#include <functional>
using std::bind;
using std::placeholders::_1;
*/
using std::string;
using std::thread;
using std::future;

#include <utility>
using std::move;

using namespace memcx::memcuv;

MemcacheUv::MemcacheUv(const string& host, const int port) {

  loop_ = uv_loop_new();
  endpoint_ = uv_ip4_addr(host.c_str(), port);
  //TODO: make pool size configurable
  connections_.emplace_back(loop_, endpoint_);
  loop_thread_ = thread([this] { LoopThread(); });
//  connections_.front().WaitOnConnect();
}

MemcacheUv::~MemcacheUv() {
  uv_async_t* async = new uv_async_t();
  uv_async_init(loop_, async, MemcacheUv::Shutdown);
  async->data = this;
  uv_async_send(async);
  loop_thread_.join();
  uv_loop_delete(loop_);
}

void MemcacheUv::Shutdown(uv_async_t* handle, int status) {
  MemcacheUv* ths = static_cast<MemcacheUv*>(handle->data);
  uv_close(reinterpret_cast<uv_handle_t*>(handle), NULL);
  delete handle;
  ths->connections_.front().Close();
}

void MemcacheUv::SendSet(unique_ptr<SetRequest> req) {
  //TODO: get good connection from pool
  connections_.front().SubmitRequest(move(req));
}

void MemcacheUv::LoopThread() {
  uv_run(loop_, UV_RUN_DEFAULT);
}
