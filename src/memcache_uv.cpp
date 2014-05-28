#include "memcache_uv.h"

#include <exception>
using std::exception;
using std::runtime_error;
#include <functional>
using std::bind;
using std::placeholders::_1;
#include <memory>
using std::unique_ptr;
#include <utility>
using std::move;

using std::chrono::milliseconds;
using std::future;
using std::mutex;
using std::string;
using std::thread;
using std::unique_lock;

#include <uv.h>

using namespace memcx::memcuv;

MemcacheUv::MemcacheUv(const string& host, 
                       const int port, 
                       const size_t pool_size) {
  live_connections_ = 0;
  loop_ = uv_loop_new();

  uv_async_init(loop_, &request_async_, SendNewMessages);
  request_async_.data = this;

  uv_async_init(loop_, &shutdown_async_, Shutdown);
  shutdown_async_.data = this;

  struct sockaddr_in endpoint_ = uv_ip4_addr(host.c_str(), port);
  UvConnection::ConnectionCallback cb(bind(&MemcacheUv::ConnectEvent, this, _1));
  for (size_t i = 0; i < pool_size; ++i) {
    connections_.emplace_back(loop_, endpoint_, cb);
  }
  cursor_ = connections_.begin();
  loop_thread_ = thread([this] { LoopThread(); });
}

MemcacheUv::~MemcacheUv() {
  uv_close(reinterpret_cast<uv_handle_t*>(&request_async_), nullptr);
  uv_async_send(&shutdown_async_);
  loop_thread_.join();
  uv_loop_delete(loop_);
}

void MemcacheUv::SendRequest(unique_ptr<Request> req) {
  {
    unique_lock<mutex> l(wait_lock_);
    new_requests_.push(req.release());
  }

  NewMessageAsync();
}

void MemcacheUv::NewMessageAsync() {
  uv_async_send(&request_async_);
}

void MemcacheUv::SendNewMessages(uv_async_t* async, int status) {
  if (status == 0) {
    MemcacheUv* ths = static_cast<MemcacheUv*>(async->data);
    unique_lock<mutex> l(ths->wait_lock_);
    try {
      while (!ths->new_requests_.empty()) {
        ConnectionIter connection = ths->NextOpenConnection();
        connection->WriteRequest(unique_ptr<Request>(ths->new_requests_.front()));
        ths->new_requests_.pop();
      }
    } catch(exception& e) {
    }
  }
}

void MemcacheUv::ConnectEvent(UvConnection* connection) {
  unique_lock<mutex> l(wait_lock_);
  if (connection->connected()) {
    ++live_connections_;
    if (!new_requests_.empty()) {
      NewMessageAsync();
    }
  } else {
    if (live_connections_ > 0) {
      --live_connections_;
    }
  }
  connect_condition_.notify_all();
}

void MemcacheUv::WaitConnect(const milliseconds& timeout) {
  unique_lock<mutex> l(wait_lock_);
  if (!connect_condition_.wait_for(l, timeout, [this](){ return live_connections_ > 0; })) {
    throw runtime_error("Connection timeout");
  }
}

ConnectionIter MemcacheUv::NextOpenConnection() {
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

  if (!cursor_->connected()) {
    throw runtime_error("Failed to find open connection");
  }

  ConnectionIter connection = cursor_;
  if (++cursor_ == connections_.end()) {
    cursor_ = connections_.begin();
  }
  return connection;
}

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
