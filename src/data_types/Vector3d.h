#pragma once

#include <blaze/math/StaticVector.h>

using Vector3d = blaze::StaticVector<float, 3UL>;

// Overwrite the default operator<< for StaticVector's.
std::ostream& operator<<(std::ostream& os, Vector3d const& vector3d);
