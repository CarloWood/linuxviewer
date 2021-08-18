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
// It also support move operations, implemented by swapping, which will correctly destruct the old resource,
// since the right value argument get destructed right after the operation
// It does not support copy operation
//
// Typical usage:
//
//   vulkan::VRaii<vk::DescriptorSetLayout> camera_descriptor_set_layout
//
//   You can allocate VRaii as class member in advance and then when you are allocating the resource
//
//   camera_descriptor_set_layout = vulkan::VRaii<vk::DescriptorSetLayout>(
//	device.createDescriptorSetLayout(create_info, nullptr),
//	[device = this->device](auto& layout){ device.destroyDescriptorSetLayout(layout); }
//   )
//
template <typename T>
class VRaii
{
 private:
  T object;
  std::function<void(T&)> deleter;

 public:
  using obj_t = T;

  VRaii() : object(nullptr), deleter([](T&) {}) {}
  VRaii(T obj, std::function<void(T&)> deleter) : object(obj), deleter(deleter) {}
  ~VRaii() { cleanup(); }

  VRaii<T>& operator=(VRaii<T> const&) = delete;
  VRaii(VRaii<T> const&)               = delete;

  // Move constructor.
  VRaii(VRaii<T>&& other) :
    object(nullptr),    // To be swapped to "other".
    deleter([](T&) {})  // deleter will be copied in case there is still use for the old container.
  {
    swap(*this, other);
  }

  T& get() { return object; }
  const T& get() const { return object; }

  operator T const &() const { return object; }

  T* data() { return &object; }

  VRaii<T>& operator=(VRaii<T>&& other)
  {
    swap(*this, other);
    return *this;
  }

  friend void swap(VRaii<T>& first, VRaii<T>& second)
  {
    std::swap(first.object, second.object);
    std::swap(first.deleter, second.deleter);
  }

 private:
  void cleanup() { deleter(object); }
};

}  // namespace vulkan
