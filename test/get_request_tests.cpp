#include <memory>
using std::unique_ptr;
#include <string>
using std::string;
using std::to_string;

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TestRunner.h>

#include "buffer.h"
#include "uv_request.h"

using memcx::Buffer;
using memcx::GetCallback;
using memcx::memcuv::GetRequest;
using memcx::memcuv::UvConnection;

class GetRequestTests : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( GetRequestTests );
  CPPUNIT_TEST( GetRequestInit );
  CPPUNIT_TEST( IngestGood );
  CPPUNIT_TEST( IngestGoodSplit );
  CPPUNIT_TEST( IngestError );
  CPPUNIT_TEST_SUITE_END();

public:
  void GetRequestInit()
  {
    bool cb_activated = false;
    GetCallback cb([&cb_activated](const string& value, const string& err) { cb_activated = true; });
    const string key = "key";
    const string value = "value";
    GetRequest g(key,cb);
    CPPUNIT_ASSERT_EQUAL(string("get key\r\n"), g.command());
    CPPUNIT_ASSERT(!g.complete());
    CPPUNIT_ASSERT_EQUAL((UvConnection*)nullptr, g.connection());
    CPPUNIT_ASSERT(!g.IsError());
    CPPUNIT_ASSERT(g.error_msg().empty());
    CPPUNIT_ASSERT(!cb_activated);
  }

  void IngestGood() {
    bool cb_activated = false;
    string cb_value;
    GetCallback cb([&](const string& value, const string& err) { 
      cb_activated = true; 
      cb_value = value;
    });
    const string expected_value = "###";
    unique_ptr<GetRequest> gr(RespondRequest("VALUE 999 0 3\r\n"+expected_value+"\r\nEND\r\n", cb));
    
    CPPUNIT_ASSERT_EQUAL(true, gr->complete());
    CPPUNIT_ASSERT(!gr->IsError());
    CPPUNIT_ASSERT(!cb_activated);

    gr->Notify();
    CPPUNIT_ASSERT(cb_activated);
    CPPUNIT_ASSERT_EQUAL(expected_value, cb_value);
  }

  void IngestGoodSplit() {
    bool cb_activated = false;
    string cb_value;
    GetCallback cb([&](const string& value, const string& err) { 
      cb_activated = true; 
      cb_value = value;
    });
    const string first_part = "FIRSTPART";
    const string second_part = "THERESTOFIT";
    const string expected_value = first_part+second_part;
    unique_ptr<GetRequest> gr(RespondRequest("VALUE 999 0 "+to_string(expected_value.size())+"\r\n"+first_part, cb));

    CPPUNIT_ASSERT(!gr->complete());
    CPPUNIT_ASSERT(!gr->IsError());

    const string second_write = second_part+"\r\nEND\r\n";
    Buffer b(second_write.size()+1);
    uv_buf_t d = b.InitRead();
    copy(second_write.begin(), second_write.end(), d.base);
    b.MarkNewBytesRead(first_part.size() + second_write.size());

    size_t bytes_read = gr->Ingest(b);
    CPPUNIT_ASSERT_EQUAL(second_write.size(), bytes_read);

    CPPUNIT_ASSERT(gr->complete());
    CPPUNIT_ASSERT(!gr->IsError());
    
    gr->Notify();
    CPPUNIT_ASSERT(cb_activated);
    CPPUNIT_ASSERT_EQUAL(expected_value, cb_value);
  }

  void IngestError() {
    string cb_value;
    string cb_error;
    GetCallback cb([&](const string& value, const string& err) { 
      cb_value = value;
      cb_error = err; 
    });
    unique_ptr<GetRequest> gr(RespondRequest("ERROR\r\n", cb));
    
    CPPUNIT_ASSERT(gr->complete());
    CPPUNIT_ASSERT(gr->IsError());
    CPPUNIT_ASSERT(cb_error.empty());

    gr->Notify();
    CPPUNIT_ASSERT_EQUAL(string("memcached: ERROR"), cb_error);
  }

private:
  GetRequest* RespondRequest(const string& response, const GetCallback& callback) {
    Buffer b(response.size()+1);
    uv_buf_t d = b.InitRead();
    copy(response.begin(), response.end(), d.base);
    b.MarkNewBytesRead(response.size());
    GetRequest* g = new GetRequest("key", callback);
    g->Ingest(b);
    return g;
  }

};

int main( int argc, char **argv)
{
  CppUnit::TextUi::TestRunner runner;
  runner.addTest( GetRequestTests::suite() );
  runner.run();
  return 0;
}
