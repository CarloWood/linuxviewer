#include "sys.h"
#include "DateTime.h"
#include "utils/AIAlert.h"
#include "debug.h"
#include "utils/debug_ostream_operators.h"

int main()
{
  Debug(debug::init());

  DateTime dt(67767976233532799);

  Dout(dc::notice, dt);

  std::string_view sv = "2147483647-12-31T23:59:59Z";
  DateTime dt2;

  try
  {
    dt2.assign_from_iso8601_string(sv);
    Dout(dc::notice, dt2);
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
  }
}
