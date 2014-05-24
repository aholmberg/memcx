#include "memcache_uv.h"
#include <uv.h>

#include <chrono>
using std::chrono::milliseconds;

#include <exception>
using std::runtime_error;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <memory>
using std::unique_ptr;

#include <functional>
using std::bind;
using std::placeholders::_1;

using std::unique_lock;
using std::mutex;

using std::string;
using std::thread;
using std::future;

#include <utility>
using std::move;

using namespace memcx::memcuv;

MemcacheUv::MemcacheUv(const string& host, const int port, const size_t pool_size) {

  live_connections_ = 0;

  loop_ = uv_loop_new();
  endpoint_ = uv_ip4_addr(host.c_str(), port);
  UvConnection::ConnectionCallback cb(bind(&MemcacheUv::ConnectEvent, this, _1));
  for (size_t i = 0; i < pool_size; ++i) {
    connections_.emplace_back(loop_, endpoint_, cb);
  }
  cursor_ = connections_.begin();
  loop_thread_ = thread([this] { LoopThread(); });
}

MemcacheUv::~MemcacheUv() {
  uv_async_t* async = new uv_async_t();
  uv_async_init(loop_, async, MemcacheUv::Shutdown);
  async->data = this;
  uv_async_send(async);
  loop_thread_.join();
  uv_loop_delete(loop_);
}

void MemcacheUv::SendSet(unique_ptr<SetRequest> req, const milliseconds& timeout) {
  unique_lock<mutex> l(wait_lock_);
  if (connect_condition_.wait_for(l, timeout, [this](){ return live_connections_ > 0; })) {
    ConnectionIter start = cursor_;
    while (!cursor_->connected()) {
      ++cursor_;
      if (cursor_ == connections_.end()) {
        cursor_ = connections_.begin();
      }
      if (cursor_ == start) {
        break;
      }
    }
    if (cursor_->connected()) {
      cursor_->SubmitRequest(move(req));
      ++cursor_;
      if (cursor_ == connections_.end()) {
        cursor_ = connections_.begin();
      }
    } else {
      throw runtime_error("Failed to find open connection");
    }
  } else {
    throw runtime_error("Connection timeout");
  }
}

void MemcacheUv::ConnectEvent(UvConnection* connection) {
  unique_lock<mutex> l(wait_lock_);
  if (connection->connected()) {
    ++live_connections_;
  } else {
    if (live_connections_ > 0) {
      --live_connections_;
    }
  }
  connect_condition_.notify_all();
}

/*
void MemcacheUv::WaitConnect() {
  unique_lock<mutex> l(wait_lock_);
  connect_condition_.wait(l, [this](){ return live_connections_ > 0; });
}
*/

void MemcacheUv::LoopThread() {
  uv_run(loop_, UV_RUN_DEFAULT);
}

void MemcacheUv::Shutdown(uv_async_t* handle, int status) {
  MemcacheUv* ths = static_cast<MemcacheUv*>(handle->data);
  uv_close(reinterpret_cast<uv_handle_t*>(handle), NULL);
  for (UvConnection& c : ths->connections_) {
    c.Close("shutdown");
  }
}
