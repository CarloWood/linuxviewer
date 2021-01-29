#include "sys.h"
#include "Vector3d.h"
#include "debug.h"

int main()
{
  Debug(debug::init());

  if (blaze::defaultTransposeFlag != blaze::columnVector)
    Dout(dc::warning, "blaze::defaultTransposeFlag != blaze::columnVector");

  if (blaze::defaultAlignmentFlag != blaze::aligned)
    Dout(dc::warning, "blaze::defaultAlignmentFlag != blaze::aligned");

  if (blaze::defaultPaddingFlag != blaze::padded)
    Dout(dc::warning, "blaze::defaultPaddingFlag != blaze::padded");

  Vector3d v1{0.5, 0.25, 0.3333333333333};

  Dout(dc::notice, "alignof(Vector3d) = " << alignof(Vector3d) << "; sizeof(Vector3d) = " << sizeof(Vector3d));
  Dout(dc::notice, "v1 = " << v1);
}
