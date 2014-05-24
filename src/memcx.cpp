#include "memcx.h"

#include <exception>
using std::runtime_error;
#include <future>
using std::future;
#include <memory>
using std::unique_ptr;
#include <string>
using std::string;

#include "memcache_uv.h"

using namespace memcx;

static memcuv::MemcacheUv* client = nullptr;

void memcx::Init(const std::string& host,
     const int port,
     const size_t pool_size) {

  if (IsInit()) {
    throw runtime_error("memcx::Init already initialized");
  }
 
  client = new memcuv::MemcacheUv(host, port, pool_size);
}

void memcx::Shutdown() {
  if (IsInit()) {
    delete client;
    client = nullptr;
  }
}

bool memcx::IsInit() {
  return client != nullptr;
}

void VerifyInit(const char* function) {
  if (!IsInit()) {
    throw runtime_error(string(function) + " called before Init");
  }
}
  
void memcx::SetSync(const string &key, const string& value, const std::chrono::milliseconds& timeout) {
  VerifyInit(__FUNCTION__);
  SetRequestAsync* req = new SetRequestAsync(key, value);
  future<void> set_future = req->GetFuture();
  //client->SendSet(static_cast<SetRequest*>(req));
  client->SendSet(unique_ptr<SetRequest>(req), timeout);
  set_future.get();
}
