#include "sys.h"
#include "LlsdDecoder.h"
#include "debug.h"

class TestLlsdDecoder : public LlsdDecoder
{
 public:
  size_t end_of_msg_finder_test(char const* new_data, size_t rlen, evio::EndOfMsgFinderResult& result)
  {
    return LlsdDecoder::end_of_msg_finder(new_data, rlen, result);
  }
};

int main()
{
  Debug(debug::init());

  TestLlsdDecoder llsd_decoder;

  std::string buf = "<n> <m>f</m> </n> \n";

  size_t const tlen = buf.size();
  evio::EndOfMsgFinderResult result;

  // l1 = 10; l2 = 15
  for (size_t l1 = 0; l1 <= tlen; ++l1)
    for (size_t l2 = l1; l2 <= tlen; ++l2)
    {
      Dout(dc::notice, "l1 = " << l1 << "; l2 = " << l2);
      char const* new_data = buf.data();
      size_t dlen = 0;
      size_t rlen = l1;
      int i = 0;
      do
      {
        Dout(dc::notice|flush_cf|continued_cf, "end_of_msg_finder_test(\"" << libcwd::buf2str(new_data, rlen) << "\", " << rlen << ", result) = ");
        size_t len = llsd_decoder.end_of_msg_finder_test(new_data, rlen, result);
        Dout(dc::finish, len);
        ASSERT(i != 0 || (len == (l1 < 12 ? 0 : l1 < 17 ? 12 : 17)));
        ASSERT(i != 1 || len == 0 || (l1 + len == (l2 < 12 ? 0 : l2 < 17 ? 12 : 17)));
        ASSERT(i != 2 || len == 0 || (l2 + len == 17));

        if (len > 0)
          dlen = (new_data - buf.data()) + len;
        new_data += rlen;
        ++i;
        if (i == 1)
          rlen = l2 - (new_data - buf.data());
        else
          rlen = tlen - (new_data - buf.data());
      }
      while (i < 3 && dlen < tlen);

      ASSERT(dlen == 17);
    }
}
