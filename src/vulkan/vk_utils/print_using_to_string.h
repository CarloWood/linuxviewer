#include <iostream>

namespace vk {

template<typename T>
concept ConceptHasVkToString = requires(T obj)
{
  to_string(obj);
};

// Use to_string for types in namespace vk when to_string is defined for those types.
template<ConceptHasVkToString T>
std::ostream& operator<<(std::ostream& os, T const& obj)
{
  os << to_string(obj);
  return os;
}

} // namespace vk
