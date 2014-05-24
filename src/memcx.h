#ifndef MEMCX_H
#define MEMCX_H

#include <chrono>
#include <functional>
#include <string>

namespace memcx {

typedef std::function<void(const std::string& err)> SetCallback;
typedef std::function<void(const std::string& value, const std::string& err)> GetCallback;

const std::chrono::milliseconds DEFAULT_TIMEOUT = std::chrono::milliseconds(25);

// call before any IO operations
void Init(const std::string& host = "localhost", 
     const int port = 11211,
     const size_t pool_size = 5,
     const std::chrono::milliseconds& connect_timeout = DEFAULT_TIMEOUT);
// call on shutdown
void Shutdown();
bool IsInit();

void SetSync(const std::string &key, const std::string &value, const std::chrono::milliseconds& timeout = DEFAULT_TIMEOUT);//TODO: flags
void SetAsync(const std::string &key, const std::string &value, SetCallback callback, const std::chrono::milliseconds& timeout = DEFAULT_TIMEOUT);

std::string GetSync(const std::string &key, const std::chrono::milliseconds& timeout = DEFAULT_TIMEOUT);
void GetAsync(const std::string &key, GetCallback callback, const std::chrono::milliseconds& timeout = DEFAULT_TIMEOUT);

}
#endif
