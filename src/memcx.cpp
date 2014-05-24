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

void memcx::Init(
     const std::string& host,
     const int port,
     const size_t pool_size, 
     const std::chrono::milliseconds& timeout) {

  if (IsInit()) {
    throw runtime_error("memcx::Init already initialized");
  }
 
  client = new memcuv::MemcacheUv(host, port, pool_size);
  client->WaitConnect(timeout);
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
  VerifyInit(__FUNCTION__);//TODO: guard too-long and zero-length keys
  SetRequestAsync* req = new SetRequestAsync(key, value);
  future<void> set_future = req->GetFuture();
  client->SendRequestSync(unique_ptr<SetRequest>(req), timeout);
  set_future.get();
}

void memcx::SetAsync(const string &key, const string& value, SetCallback callback, const std::chrono::milliseconds& timeout) {
  VerifyInit(__FUNCTION__);
  SetRequest* req = new SetRequest(key, value, callback);
  client->SendRequestAsync(unique_ptr<SetRequest>(req));
}

string memcx::GetSync(const std::string &key, const std::chrono::milliseconds& timeout) {
  VerifyInit(__FUNCTION__);
  GetRequestAsync* req = new GetRequestAsync(key);
  future<string> get_future = req->GetFuture();
  client->SendRequestSync(unique_ptr<GetRequest>(req), timeout);
  return get_future.get();
}

void memcx::GetAsync(const std::string &key, GetCallback callback, const std::chrono::milliseconds& timeout) {
  VerifyInit(__FUNCTION__);
  GetRequest* req = new GetRequest(key, callback);
  client->SendRequestAsync(unique_ptr<GetRequest>(req));
}

