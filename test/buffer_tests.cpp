#include <string>
using std::string;
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TestRunner.h>
#include "buffer.h"
using memcx::Buffer;

class BufferTests : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( BufferTests );
  CPPUNIT_TEST( ConstructInit );
  CPPUNIT_TEST( ReadLinePart );
  CPPUNIT_TEST( ReadLineFull );
  CPPUNIT_TEST( ReadLineIncomplete );
  CPPUNIT_TEST( Shift );
  CPPUNIT_TEST_SUITE_END();

public:
  void ConstructInit()
  {
    const size_t total_size = 8;
    Buffer b(total_size);
    uv_buf_t descriptor = b.InitRead();
    CPPUNIT_ASSERT_EQUAL(descriptor.len, total_size-1);
  }

  void ReadLinePart()
  {
    Buffer b(8);
    uv_buf_t descriptor = b.InitRead();

    const string data(descriptor.len - Buffer::ENDL_LEN - 1, '#');
    const string data_endl = data + Buffer::ENDL;

    CPPUNIT_ASSERT(data_endl.size() < descriptor.len);

    copy(data_endl.begin(), data_endl.end(), descriptor.base);
    b.MarkNewBytesRead(data_endl.size());
    string line;
    const size_t bytes_read = b.ReadLine(line);
     
    CPPUNIT_ASSERT_EQUAL(data_endl.size(), bytes_read);
    CPPUNIT_ASSERT_EQUAL(data, line);
  }

  void ReadLineFull()
  {
    Buffer b(8);
    uv_buf_t descriptor = b.InitRead();

    const string data(descriptor.len - Buffer::ENDL_LEN, '#');
    const string data_endl = data + Buffer::ENDL;

    CPPUNIT_ASSERT_EQUAL(descriptor.len, data_endl.size());

    copy(data_endl.begin(), data_endl.end(), descriptor.base);
    b.MarkNewBytesRead(data_endl.size());
    string line;
    const size_t bytes_read = b.ReadLine(line);
     
    CPPUNIT_ASSERT_EQUAL(data_endl.size(), bytes_read);
    CPPUNIT_ASSERT_EQUAL(data, line);
  }

  void ReadLineIncomplete()
  {
    Buffer b(8);
    uv_buf_t descriptor = b.InitRead();

    const string data(descriptor.len - Buffer::ENDL_LEN + 1, '#');
    const string data_endl = data + Buffer::ENDL;

    CPPUNIT_ASSERT_EQUAL(descriptor.len + 1, data_endl.size());

    copy(data_endl.begin(), data_endl.end() - 1, descriptor.base);
    b.MarkNewBytesRead(data_endl.size() - 1);
    string line;
    const size_t bytes_read = b.ReadLine(line);
     
    CPPUNIT_ASSERT_EQUAL(0UL, bytes_read);
    CPPUNIT_ASSERT(line.empty());
  }

  void Shift()
  {
    Buffer b(16);
    uv_buf_t descriptor = b.InitRead();

    const string line1(2, '1');
    const string line2(2, '2');
    const string line3(2, '3');

    string tmp = line1 + Buffer::ENDL;
    copy(tmp.begin(), tmp.end(), descriptor.base);
    size_t offset = tmp.size();
    b.MarkNewBytesRead(offset);

    tmp = line2 + Buffer::ENDL;
    copy(tmp.begin(), tmp.end(), descriptor.base + offset);
    b.MarkNewBytesRead(tmp.size());

    string line;
    b.ReadLine(line);
    CPPUNIT_ASSERT_EQUAL(line1, line);

    uv_buf_t descriptor2 = b.InitRead();
    tmp = line3 + Buffer::ENDL;
    copy(tmp.begin(), tmp.end(), descriptor2.base);
    b.MarkNewBytesRead(tmp.size());

    b.ReadLine(line);
    CPPUNIT_ASSERT_EQUAL(line2, line);

    b.ReadLine(line);
    CPPUNIT_ASSERT_EQUAL(line3, line);

    size_t bytes_read = b.ReadLine(line);
    CPPUNIT_ASSERT_EQUAL(0UL, bytes_read);
    CPPUNIT_ASSERT(line.empty());
  }
};

int main( int argc, char **argv)
{
  CppUnit::TextUi::TestRunner runner;
  runner.addTest( BufferTests::suite() );
  runner.run();
  return 0;
}
