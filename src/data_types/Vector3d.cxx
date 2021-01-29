#include "sys.h"
#include "Vector3d.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, Vector3d const& vector3d)
{
  return os << '{' << vector3d[0] << ", " << vector3d[1] << ", " << vector3d[2] << '}';
}
