#pragma once

#include "threadsafe/aithreadsafe.h"

namespace vk_utils {

template<typename T>
struct WriteLockOnly : aithreadsafe::Wrapper<T, aithreadsafe::policy::Primitive<std::mutex>>
{
 public:
  using aithreadsafe::Wrapper<T, aithreadsafe::policy::Primitive<std::mutex>>::Wrapper;

 private:
  using aithreadsafe::Wrapper<T, aithreadsafe::policy::Primitive<std::mutex>>::crat;
  using aithreadsafe::Wrapper<T, aithreadsafe::policy::Primitive<std::mutex>>::rat;
};

} // namespace vk_utils
