#include "memcx.h"

#include <exception>
using std::invalid_argument;
using std::runtime_error;
#include <future>
using std::future;
#include <memory>
using std::unique_ptr;
#include <string>
using std::string;
#include <sstream>
using std::stringstream;

#include "memcache_uv.h"

using namespace memcx;
using namespace memcuv;

static memcuv::MemcacheUv* client = nullptr;

void memcx::Init(
     const string& host,
     const int port,
     const size_t pool_size, 
     const uint64_t timeout_ms) {

  if (IsInit()) {
    throw runtime_error("memcx::Init already initialized");
  }
 
  client = new memcuv::MemcacheUv(host, port, pool_size);
  client->WaitConnect(timeout_ms);
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

void Validate(const string& key, const char* function) {
  if (!IsInit()) {
    throw runtime_error(string(function) + " called before Init");
  }

  if (key.empty() || key.size() > MAX_KEY_LEN) {
    stringstream ss;
    ss << "Invalid key: \"" << key << "\"; size must be [1," << MAX_KEY_LEN << "]";
    throw invalid_argument(ss.str());
  }
 
  if (key.find(' ') != string::npos) {
    stringstream ss;
    ss << "Invalid key: \"" << key << "\"; whitespace is not supported";
    throw invalid_argument(ss.str());
  }
}
  
void memcx::SetSync(const string& key, 
                    const string& value, 
                    const uint64_t timeout_ms) {
  Validate(key, __FUNCTION__);
  SetRequestAsync* req = new SetRequestAsync(key, value);
  future<void> set_future = req->GetFuture();
  client->SendRequest(unique_ptr<SetRequest>(req), timeout_ms);
  set_future.get();
}

void memcx::SetAsync(const string& key, 
                     const string& value, 
                     const SetCallback& callback, 
                     const uint64_t timeout_ms) {
  Validate(key, __FUNCTION__);
  SetRequest* req = new SetRequest(key, value, callback, timeout_ms);
  client->SendRequest(unique_ptr<SetRequest>(req), timeout_ms);
}

string memcx::GetSync(const string& key, const uint64_t timeout_ms) {
  Validate(key, __FUNCTION__);
  GetRequestAsync* req = new GetRequestAsync(key);
  future<string> get_future = req->GetFuture();
  client->SendRequest(unique_ptr<GetRequest>(req), timeout_ms);
  return get_future.get();
}

void memcx::GetAsync(const string& key, 
                     const GetCallback& callback, 
                     const uint64_t timeout_ms) {
  Validate(key, __FUNCTION__);
  GetRequest* req = new GetRequest(key, callback);
  client->SendRequest(unique_ptr<GetRequest>(req), timeout_ms);
}
