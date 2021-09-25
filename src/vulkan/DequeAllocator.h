#pragma once

#include <limits>
#include <cstdlib>
#include <type_traits>
#include "debug.h"

template<typename T, typename DataType = T>
class DequeAllocator
{
 private:
  utils::NodeMemoryResource* m_node_memory_resource;

 public:
  using value_type = T;

  DequeAllocator(utils::NodeMemoryResource& node_memory_resource) : m_node_memory_resource(&node_memory_resource) { }

  template<typename U>
  DequeAllocator(DequeAllocator<U, DataType> const&) : m_node_memory_resource(nullptr)
  {
    Dout(dc::notice, "Constructing DequeAllocator<" << type_info_of<T>().demangled_name() << ", " << type_info_of<DataType>().demangled_name() <<
        "> from DequeAllocator<" << type_info_of<U>().demangled_name() << ", " << type_info_of<DataType>().demangled_name() << ">");
  }

  [[nodiscard]] T* allocate(std::size_t number_of_objects);
  void deallocate(T* p, std::size_t n) noexcept;
};

template<typename T, typename DataType>
T* DequeAllocator<T, DataType>::allocate(std::size_t n)
{
  DoutEntering(dc::notice, "DequeAllocator<" << type_info_of<T>().demangled_name() << ", " << type_info_of<DataType>().demangled_name() << ">::allocate(" << n << ")");

  if constexpr (std::is_same_v<T, DataType>)
  {
    Dout(dc::notice, "Use node allocator!");
    return static_cast<T*>(m_node_memory_resource->allocate(sizeof(T)));
  }
  else
  {
    Dout(dc::notice, "Use malloc");

    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw std::bad_array_new_length();

    //AIMemoryPagePool::instance()

    if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
    {
      Dout(dc::notice, "returning pointer " << p);
      return p;
    }

    throw std::bad_alloc();
  }
}

template<typename T, typename DataType>
void DequeAllocator<T, DataType>::deallocate(T* p, std::size_t n) noexcept
{
  DoutEntering(dc::notice, "DequeAllocator<" << type_info_of<T>().demangled_name() << ", " << type_info_of<DataType>().demangled_name() << ">::deallocate(" << (void*)p << ", " << n << ")");

  if constexpr (std::is_same_v<T, DataType>)
  {
    Dout(dc::notice, "Use node allocator!");
    m_node_memory_resource->deallocate(p);
  }
  else
  {
    Dout(dc::notice, "Use free");

    std::free(p);
  }
}

template<typename T, typename DataType>
bool operator==(DequeAllocator<T, DataType> const& a1, DequeAllocator<T, DataType> const& a2)
{
  return a1.m_node_memory_resource == a2.m_node_memory_resource;
}

template<typename T, typename DataType>
bool operator!=(DequeAllocator<T, DataType> const& a1, DequeAllocator<T, DataType> const& a2)
{
  return !(a1 == a2);
}
