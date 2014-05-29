#ifndef MEMCX_H
#define MEMCX_H

#include <functional>
#include <string>

namespace memcx {

typedef std::function<void(const std::string& err)> SetCallback;
typedef std::function<void(const std::string& value, 
                           const std::string& err)> GetCallback;

const size_t MAX_KEY_LEN = 250;
const uint64_t DEFAULT_CONNECT_TIMEOUT = 500;
const uint64_t DEFAULT_REQUEST_TIMEOUT = 0;// never

// call before any IO operations
void Init(const std::string& host = "localhost", 
          const int port = 11211,
          const size_t pool_size = 5,
          const uint64_t connect_timeout_ms = DEFAULT_CONNECT_TIMEOUT);

// call on shutdown
void Shutdown();

bool IsInit();

void SetSync(const std::string &key, 
             const std::string &value, 
             const uint64_t timeout_ms = DEFAULT_REQUEST_TIMEOUT);

void SetAsync(const std::string &key, 
              const std::string &value, 
              const SetCallback& callback, 
              const uint64_t timeout_ms = DEFAULT_REQUEST_TIMEOUT);

std::string GetSync(const std::string &key, 
                    const uint64_t timeout_ms = DEFAULT_REQUEST_TIMEOUT);

void GetAsync(const std::string &key, 
              const GetCallback& callback, 
              const uint64_t timeout_ms = DEFAULT_REQUEST_TIMEOUT);

}
#endif
