#include "sys.h"
#include "GridInfoDecoder.h"
#include "GridInfo.h"
#ifdef CWDEBUG
#include "xml/Writer.h"
#endif

size_t GridInfoDecoder::end_of_msg_finder(char const* new_data, size_t rlen, evio::EndOfMsgFinderResult& result)
{
  DoutEntering(dc::io, "GridInfoDecoder::end_of_msg_finder(..., " << rlen << ")");

  if (AI_UNLIKELY(m_message_length == -1))        // Not initialized yet.
    m_message_length = m_get_message_length();

  m_total_len += rlen;

  if (m_total_len < m_message_length)
    return 0;

  result.m_new_decoder = &m_next_decoder;
  result.m_sink_type = evio::decoder_stream_sink;
  return m_message_length;
}

void GridInfoDecoder::decode(int& allow_deletion_count)
{
  DoutEntering(dc::notice, "GridInfoDecoder::decode({" << allow_deletion_count << "})");

  xml::Reader reader;
  try
  {
    reader.parse(*this, 1);
    m_grid_info.xml(reader);
  }
  catch (AIAlert::Error const& error)
  {
    close_input_device(allow_deletion_count);
    throw;
  }
  catch (std::runtime_error const& error)
  {
    close_input_device(allow_deletion_count);
    THROW_FALERT("XML parse error: [ERROR]", AIArgs("[ERROR]", error.what()));
  }

  Debug(libcw_do.off());
  xml::Writer writer(std::cout);
  writer.write(m_grid_info);
  Debug(libcw_do.on());
}
