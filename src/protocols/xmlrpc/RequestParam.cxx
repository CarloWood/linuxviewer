#include "sys.h"
#include "RequestParam.h"
#include <iostream>
#include "debug.h"

namespace xmlrpc {

void RequestParam::write_param(std::ostream& output) const
{
  output << "<param />";
}

} // namespace xmlrpc
