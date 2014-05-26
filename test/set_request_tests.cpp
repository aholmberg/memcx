#include <memory>
using std::unique_ptr;
#include <string>
using std::string;

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TestRunner.h>

#include "buffer.h"
#include "request.h"

using memcx::Buffer;
using memcx::SetCallback;
using memcx::SetRequest;

class SetRequestTests : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( SetRequestTests );
  CPPUNIT_TEST( SetRequestInit );
  CPPUNIT_TEST( IngestGood );
  CPPUNIT_TEST( IngestGoodSplit );
  CPPUNIT_TEST( IngestError );
  CPPUNIT_TEST( IngestMultiLine );
  CPPUNIT_TEST( MultipleError );
  CPPUNIT_TEST_SUITE_END();

public:
  void SetRequestInit()
  {
    bool cb_activated  = false;
    SetCallback cb([&cb_activated](const string& err) { cb_activated = false; });
    const string key = "key";
    const string value = "value";
    const unsigned int flag = 1;
    const unsigned int exp = 2;
    SetRequest s(key, value, cb, flag, exp);
    CPPUNIT_ASSERT_EQUAL(string("set key 1 2 5\r\nvalue\r\n"), s.command());
    CPPUNIT_ASSERT(!s.complete());
    CPPUNIT_ASSERT_EQUAL((void*)nullptr, s.data());
    CPPUNIT_ASSERT(!s.IsError());
    CPPUNIT_ASSERT(s.error_msg().empty());
    CPPUNIT_ASSERT(!cb_activated);
  }

  void IngestGood() {
    bool cb_activated  = false;
    SetCallback cb([&cb_activated](const string& err) { cb_activated = true; });
    unique_ptr<SetRequest> sr(RespondRequest("STORED\r\n", cb));
    
    CPPUNIT_ASSERT_EQUAL(true, sr->complete());
    CPPUNIT_ASSERT(!sr->IsError());
    CPPUNIT_ASSERT(!cb_activated);

    sr->Notify();
    CPPUNIT_ASSERT(cb_activated);
  }

  void IngestGoodSplit() {
    const string first = "STOR";
    const string second = "ED\r\n";
    SetRequest s("key", "value", [](const string&){}, 1, 2);

    Buffer b(16);
    uv_buf_t d = b.InitRead();
    copy(first.begin(), first.end(), d.base);
    b.MarkNewBytesRead(first.size());

    size_t bytes_read = s.Ingest(b);
    CPPUNIT_ASSERT_EQUAL(0UL, bytes_read);
    CPPUNIT_ASSERT(!s.complete());
    CPPUNIT_ASSERT(!s.IsError());

    d = b.InitRead();
    copy(second.begin(), second.end(), d.base);
    b.MarkNewBytesRead(second.size());
    bytes_read = s.Ingest(b);
    CPPUNIT_ASSERT_EQUAL(8UL, bytes_read);
    CPPUNIT_ASSERT(s.complete());
    CPPUNIT_ASSERT(!s.IsError());
  }

  void IngestError() {
    string error_received;
    SetCallback cb([&error_received](const string& err) { error_received = err; });
    unique_ptr<SetRequest> sr(RespondRequest("ERROR\r\n", cb));
    
    CPPUNIT_ASSERT_EQUAL(true, sr->complete());
    CPPUNIT_ASSERT_EQUAL(true, sr->IsError());
    CPPUNIT_ASSERT(error_received.empty());

    sr->Notify();
    CPPUNIT_ASSERT_EQUAL(string("memcached: ERROR"), error_received);
  }

  void IngestMultiLine() {
    string error_received;
    SetCallback cb([&error_received](const string& err) { error_received = err; });
    unique_ptr<SetRequest> sr(RespondRequest("CLIENT_ERROR bad command line format\r\n\r\nERROR\r\n", cb));
    
    CPPUNIT_ASSERT_EQUAL(true, sr->complete());
    CPPUNIT_ASSERT_EQUAL(true, sr->IsError());
    CPPUNIT_ASSERT(error_received.empty());

    sr->Notify();
    CPPUNIT_ASSERT_EQUAL(string("CLIENT_ERROR bad command line format"), error_received);
  }

  void MultipleError() {
    SetRequest s("key", "value", [](const string&){}, 1, 2);
    const string expect_first = "one";
    s.set_error_msg(expect_first);
    s.set_error_msg("two");
    CPPUNIT_ASSERT_EQUAL(expect_first, s.error_msg());
  }

private:
  SetRequest* RespondRequest(const string& response, const SetCallback& callback) {
    Buffer b(response.size()+1);
    uv_buf_t d = b.InitRead();
    copy(response.begin(), response.end(), d.base);
    b.MarkNewBytesRead(response.size());
    SetRequest* s = new SetRequest("key", "value", callback, 1, 2);
    s->Ingest(b);
    return s;
  }

};

int main( int argc, char **argv)
{
  CppUnit::TextUi::TestRunner runner;
  runner.addTest( SetRequestTests::suite() );
  runner.run();
  return 0;
}
