#include "memcx.h"
#include <chrono>
#include <iostream>
using namespace std;

int main(const int argc, const char* argv[]) {

  try {
    memcx::Init("127.0.0.1");
    memcx::SetSync("asdf", "hello");
    for (int i = 1; i < 1000; ++i) {
      memcx::SetSync(to_string(i), to_string(i));
    }
    cout << "success\n";
  } catch(exception& e) {
    cerr << "exception: " << e.what() << endl;
  }
  return 0;
}
