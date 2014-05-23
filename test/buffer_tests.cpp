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
	CPPUNIT_TEST( ReadLineFull );
	CPPUNIT_TEST_SUITE_END();

public:
	void ConstructInit()
	{
      const size_t total_size = 8;
      Buffer b(8);
      uv_buf_t descriptor = b.InitRead();
	  CPPUNIT_ASSERT_EQUAL(descriptor.len, total_size-1);
	}

	void ReadLineFull()
	{
      const size_t total_size = 8;
      Buffer b(8);
      uv_buf_t descriptor = b.InitRead();

      const string data(descriptor.len - Buffer::ENDL_LEN, '#');
      const string data_endl = data + Buffer::ENDL;

      CPPUNIT_ASSERT(data_endl.size() == descriptor.len);

      copy(data_endl.begin(), data_endl.end(), descriptor.base);
      b.MarkNewBytesRead(data_endl.size());
      string line;
      const size_t bytes_read = b.TryReadLine(line);
     
	  CPPUNIT_ASSERT_EQUAL(data_endl.size(), bytes_read);
	  CPPUNIT_ASSERT_EQUAL(data, line);
	}
};

int main( int argc, char **argv)
{
  CppUnit::TextUi::TestRunner runner;
  runner.addTest( BufferTests::suite() );
  runner.run();
  return 0;
}
