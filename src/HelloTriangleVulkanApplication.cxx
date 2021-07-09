#include "sys.h"
#include "debug.h"

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  Dout(dc::notice, "Entering main()");
  Dout(dc::notice, "Leaving main()");
}
