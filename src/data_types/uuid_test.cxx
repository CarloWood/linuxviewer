#include "sys.h"
#include "UUID.h"
#include "debug.h"

int main()
{
  Debug(debug::init());

  std::string s("359062b2-acea-4dda-aa3f-28fa4ea987ce");
  std::string_view const sv(s);

  UUID uuid(sv);

  Dout(dc::notice, uuid);
}
