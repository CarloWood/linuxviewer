#include "sys.h"
#include "RegionPositionLookAt.h"
#include "protocols/xmlrpc/initialize.h"
#include "utils/AIAlert.h"
#include "debug.h"

// A RegionPositionLookAt is initialized from XML RPC with a string like:
// {'region_handle':[r254208,r261120], 'position':[r42.90096,r47.87543,r23.00385], 'look_at':[r-0.9883952,r-0.02419114,r0.1499664]}
void RegionPositionLookAt::assign_from_string(std::string_view data)
{
  //FIXME: write a parser that can deal with "{...}" containing a comma separated list of 'key':value pairs,
  // and match 'key' against a utils::Dictionary that maps them to the right member variables.
  bool parse_error = false;
  for (int i = 0; i < 3; ++i)
  {
    size_t pos1 = data.find('[');
    if (pos1 == std::string_view::npos)
    {
      parse_error = true;
      break;
    }
    data.remove_prefix(pos1);
    size_t pos2 = data.find(']');
    if (pos2 == std::string_view::npos)
    {
      parse_error = true;
      break;
    }
    switch (i)
    {
      case 0:
        xmlrpc::initialize(m_region_handle, data);
        break;
      case 1:
        xmlrpc::initialize(m_position, data);
        break;
      case 2:
        xmlrpc::initialize(m_look_at, data);
        break;
    }
    data.remove_prefix(pos2);
  }
  if (parse_error)
    THROW_FALERT("Invalid characters [[DATA]]", AIArgs("[DATA]", utils::print_using(data, utils::c_escape)));
}
