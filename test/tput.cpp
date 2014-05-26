#include "memcx.h"
#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono;

int main(const int argc, const char* argv[]) {

  if (argc < 4 ) {
    cerr << "usage: " << argv[0] << " <pool_size> <client_thread_count> <ops_per_thread>\n";
    exit(1);
  }

  unsigned int pool_size = strtoul(argv[1], nullptr, 10);
  unsigned int client_thread_count = strtoul(argv[2], nullptr, 10);
  unsigned int ops_per_thread = strtoul(argv[3], nullptr, 10);

  try {
    memcx::Init("127.0.0.1", 11211, pool_size);

    vector<future<void>> futures;
    futures.reserve(client_thread_count);

    system_clock::time_point start = system_clock::now();
    for (unsigned int id = 0; id < client_thread_count; ++id) {
      futures.push_back(async(launch::async, [id, ops_per_thread]() {
        string tid = to_string(id);
        string key;
        string val;
        string read;
        for (unsigned int i = 0; i < ops_per_thread; ++i) {
          val = to_string(i);
          key = tid+val;
          memcx::SetSync(key, val);
          read = memcx::GetSync(key);
          if (read != val) {
            stringstream ss;
            ss << key << " mismatch iter " << val << "(" << read << ")";
            throw runtime_error(ss.str());
          }
        }
      }));
    }
 
    for_each(futures.begin(), futures.end(), [](future<void>& f) { f.get(); });

    system_clock::time_point end = system_clock::now();

    cout << duration_cast<milliseconds>(end-start).count() << endl;

  } catch(exception& e) {
    cout << "exception: " << e.what() << endl;
  }
  memcx::Shutdown();
  return 0;
}
