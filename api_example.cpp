#include "memcx.h"
#include <chrono>
#include <future>
#include <iostream>

using namespace std;

int KEYS = 10;

void SetSync() {
  cout << "SetSync: ";
  try {
    memcx::SetSync("asdf", "hello");
    for (int i = 0; i < KEYS; ++i) {
      string kv = to_string(i);
      memcx::SetSync(kv, kv);
    }
    cout << "set " << KEYS << " keys\n";
  } catch(exception& e) {
    cout << "SetSync failed: " << e.what() << endl;
  }
}

void SetAsync() {
  cout << "SetAsync: ";
  promise<void> set_promise;
  auto set_future = set_promise.get_future();
  memcx::SetAsync("aysnc", "value", 
    [&set_promise](const std::string err) {
      if (err.empty()) {
        set_promise.set_value();
      } else {
        set_promise.set_exception(make_exception_ptr(runtime_error(err)));
      }
    });
  try {
    set_future.get();
    cout << "returned\n";
  } catch(exception& e) {
    cout << "failed: " << e.what() << endl;
  }
}

void GetSync() {
  cout << "GetSync:\n";
  try {
    string asdf = memcx::GetSync("asdf");
    cout << "asdf=" << asdf << endl;
    for (int i = 0; i < KEYS; ++i) {
      string kv = to_string(i);
      cout << kv << "=" << memcx::GetSync(kv) << endl;
    }
  } catch(exception& e) {
    cout << "failed: " << e.what() << endl;
  }
}

void GetAsync() {
  cout << "GetAync: ";
  promise<string> get_promise;
  auto get_future = get_promise.get_future();
  memcx::GetAsync("aysnc",
    [&get_promise](const std::string& value, const std::string err) {
      if (err.empty()) {
        get_promise.set_value(value);
      } else {
        get_promise.set_exception(make_exception_ptr(runtime_error(err)));
      }
    });
  try {
    cout << "async=" << get_future.get() << endl;
  } catch(exception& e) {
    cout << "failed: " << e.what() << endl;
  }
}

int main(const int argc, const char* argv[]) {

  try {
    memcx::Init("127.0.0.1");

    SetSync();
    SetAsync();
    GetSync();
    GetAsync();

    cout << "success\n";
  } catch(exception& e) {
    cout << "exception: " << e.what() << endl;
  }
  memcx::Shutdown();
  return 0;
}
