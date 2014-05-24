#ifndef MEMCX_H
#define MEMCX_H

#include <chrono>
#include <functional>
#include <string>

namespace memcx {

typedef std::function<void(const std::string& err)> SetCallback;
typedef std::function<void(const std::string& value, const std::string& err)> GetCallback;
// call before any IO operations
void Init(const std::string& host = "localhost", 
     const int port = 11211,
     const size_t pool_size = 5);
// call on shutdown
void Shutdown();
bool IsInit();

void SetSync(const std::string &key, const std::string &value, const std::chrono::milliseconds& timeout = std::chrono::milliseconds(250));//TODO: flags
void SetAsync(const std::string &key, const std::string &value, SetCallback callback);

std::string GetSync(const std::string &key);
void GetAsync(const std::string &key, int callback);

}
#endif
