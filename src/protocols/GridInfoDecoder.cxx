#include "sys.h"
#include "GridInfoDecoder.h"
#include "GridInfo.h"
#ifdef CWDEBUG
#include "xml/Writer.h"
#endif

void GridInfoDecoder::decode(int& allow_deletion_count, size_t msg_len)
{
  DoutEntering(dc::decoder, "GridInfoDecoder::decode({" << allow_deletion_count << "}, " << msg_len << ")");

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

#ifdef CWDEBUG
  Debug(libcw_do.off());
  xml::Writer writer(std::cout);
  writer.write(m_grid_info);
  Debug(libcw_do.on());
#endif
}
