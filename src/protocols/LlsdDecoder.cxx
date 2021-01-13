#include "sys.h"
#include "LlsdDecoder.h"
#include "debug.h"
#include <cstring>

#if 0
// Return till the last </nodename>
//                                ^__ include that character.
//
// The content of the buffer could look like this:
//
size_t LlsdDecoder::end_of_msg_finder_stream(char const* new_data, size_t rlen)
{
  if (AI_UNLIKELY(rlen == 0))
    return 0;

  bool last_char_is_angle_open_bracket = m_last_char_is_angle_open_bracket;
  m_last_char_is_angle_open_bracket = new_data[rlen - 1] == '<';
  bool inside_closing_tag = m_inside_closing_tag;
  m_inside_closing_tag = false;

  for (;;)
  {
    // For example, first loop iteration:
    //
    //  .-- new_data
    //  v
    // "...</x>./..."
    //  123456789012  rlen = 12
    //          ^
    //          '-- slash
    //
    // Second loop iteration:
    //
    //  .-- new_data
    //  v
    // "...</x>."
    //  12345678      rlen = 8
    //     ^
    //     '-- slash
    //
    char const* slash = static_cast<char const*>(memrchr(new_data, '/', rlen));

    if (AI_UNLIKELY(!slash))
    {
      // Was the previous data of the form "</..." (no '>')?
      if (inside_closing_tag)
      {
        char const* close_bracket = static_cast<char const*>(memchr(new_data, '>', rlen));
        if (!close_bracket)
          m_inside_closing_tag = true;
        return close_bracket ? close_bracket - new_data + 1 : 0;
      }
      return 0;
    }
    // The length of the string up till the found slash.
    size_t slen = slash - new_data;                                             // First loop iteration: slen = 8. Second loop iteration: slen = 4.

    if ((slash != new_data && slash[-1] == '<') ||                              // "..</..."
        AI_UNLIKELY(slash == new_data && last_char_is_angle_open_bracket))      // "...<"  "/..."
    {
      rlen -= slen;                                                             // Second loop iteration: rlen = 8 - 4 = 4, for the string "/x>.".
      char const* close_bracket = static_cast<char const*>(memchr(slash, '>', rlen));   //                                                  ^ ^
                                                                                        //                                                  | '- close_bracket
                                                                                        //                                                  `- slash
      if (close_bracket)
        return close_bracket - new_data + 1;
      m_inside_closing_tag = true;
    }

    // Shorten the data to only include everything up till the found slash.
    // So, rlen = 8 in the case of "...</.../..." and slash pointing to the right-most slash.
    // This means we go to the top as if new_data is "...</...".
    rlen = slash - new_data;
  }
}
#endif

void LlsdDecoder::decode(int& allow_deletion_count, size_t msg_len)
{
  DoutEntering(dc::notice, "LlsdDecoder::decode({" << allow_deletion_count << "}, " << msg_len << ")");

  std::vector<char> s(msg_len);
  rdbuf()->sgetn(&s[0], msg_len);
  Dout(dc::notice, "Received: \"" << libcwd::buf2str(s.data(), msg_len) << "\"");
}
