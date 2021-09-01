// This is a copy from https://github.com/WindyDarian/Vulkan-Forward-Plus-Renderer/blob/3fbd16d92290be9717cf83f4f24f83f80117074e/src/renderer/raii.h
// See https://github.com/WindyDarian/Vulkan-Forward-Plus-Renderer/blob/3fbd16d92290be9717cf83f4f24f83f80117074e/LICENSE for the license.

// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include <vulkan/vulkan.h>

#include <functional>

namespace vulkan {

// This RAII class is used to hold Vulkan handles; just like a std::unique_ptr, but for Vulkan handles instead of pointers.
//
// It will call deleter upon destruction.
// It also supports move operations, implemented by swapping, which will correctly destruct the old resource,
// since the right value argument get destructed right after the operation. It does not support copy operation.
//
// Typical usage:
//
//   vulkan::unique_handle<vk::DescriptorSetLayout> uh_camera_descriptor_set_layout
//
//   You can allocate unique_handle as class member in advance and then when you are allocating the resource
//
//   uh_camera_descriptor_set_layout = vulkan::unique_handle<vk::DescriptorSetLayout>(
//	vh_device.createDescriptorSetLayout(create_info, nullptr),
//	[vh_device = this->vh_device](auto& vh_layout){ vh_device.destroyDescriptorSetLayout(vh_layout); }
//   )
//
template <typename T>
class unique_handle
{
 private:
  T object;
  std::function<void(T&)> deleter;

 public:
  using obj_t = T;

  unique_handle() : object(nullptr), deleter([](T&) {}) { }
  unique_handle(T obj, std::function<void(T&)> deleter) : object(obj), deleter(deleter) { }
  ~unique_handle() { cleanup(); }

  unique_handle<T>& operator=(unique_handle<T> const&) = delete;
  unique_handle(unique_handle<T> const&)               = delete;

  // Move constructor.
  unique_handle(unique_handle<T>&& other) :
    object(nullptr),    // To be swapped to "other".
    deleter([](T&) {})  // deleter will be copied in case there is still use for the old container.
  {
    swap(*this, other);
  }

  // Get a reference to the underlaying Vulkan Handle.
  T& get() { return object; }
  T const& get() const { return object; }

  // Get a pointer to the underlaying Vulkan Handle.
  T* operator->() { return &object; }
  T const* operator->() const { return &object; }

  // Allow using the unary * operator to get a const-reference.
  T const& operator*() const { return object; }

  unique_handle<T>& operator=(unique_handle<T>&& other)
  {
    swap(*this, other);
    return *this;
  }

  friend void swap(unique_handle<T>& first, unique_handle<T>& second)
  {
    std::swap(first.object, second.object);
    std::swap(first.deleter, second.deleter);
  }

 private:
  void cleanup() { deleter(object); }
};

}  // namespace vulkan
