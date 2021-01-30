#include "sys.h"
#include "xmlrpc_initialize.h"
#include "utils/AIAlert.h"
#include "utils/debug_ostream_operators.h"
#include "debug.h"

int main()
{
  Debug(debug::init());

  std::string base64_str = "SGVsbG8gd29ybGQ=";
  std::string iso8601_str = "2021-01-29T15:37:23Z";
  std::string uri_str = "https://misfitzgrid.com/members/index.php?option=com_opensim&amp;view=guide&amp;tmpl=component";
  std::string uuid_str = "f1907865-23a7-43ca-b149-360242522657";
  std::string vector3d_str = "[r0.8928223,r0.450409,r0]";

  try
  {
    BinaryData b1;
    xmlrpc::initialize(b1, base64_str);
    Dout(dc::notice, "b1 = " << b1);

    DateTime d1;
    xmlrpc::initialize(d1, iso8601_str);
    Dout(dc::notice, "d1 = " << d1);

    URI u1;
    xmlrpc::initialize(u1, uri_str);
    Dout(dc::notice, "u1 = " << u1 << "; host = \"" << u1.get_host() << "\"; port = " << u1.get_port());

    UUID u2;
    xmlrpc::initialize(u2, uuid_str);
    Dout(dc::notice, "u2 = " << u2);

    Vector3d v1;
    xmlrpc::initialize(v1, vector3d_str);
    Dout(dc::notice, "v1 = " << v1);
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
  }
}
