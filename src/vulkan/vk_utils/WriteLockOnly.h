#pragma once

#include "threadsafe/threadsafe.h"

namespace vk_utils {

template<typename T>
struct WriteLockOnly : threadsafe::Unlocked<T, threadsafe::policy::Primitive<std::mutex>>
{
 public:
  using threadsafe::Unlocked<T, threadsafe::policy::Primitive<std::mutex>>::Unlocked;

 private:
  using typename threadsafe::Unlocked<T, threadsafe::policy::Primitive<std::mutex>>::crat;
  using typename threadsafe::Unlocked<T, threadsafe::policy::Primitive<std::mutex>>::rat;
};

} // namespace vk_utils
