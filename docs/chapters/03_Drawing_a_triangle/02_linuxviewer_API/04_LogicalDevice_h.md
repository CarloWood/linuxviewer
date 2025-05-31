---
layout: chapter
title: Linuxviewer API
toc_title: LogicalDevice.h
utterance_id: lv_API_LogicalDevice_h_draft
---
{% assign drawing_a_triangle = site.baseurl | append: '/chapters/03_Drawing_a_triangle' -%}

## {% include link_to_github folder="examples/hello_triangle/" name="LogicalDevice.h" %}

```cpp
#pragma once

#include <vulkan/LogicalDevice.h>

class LogicalDevice : public vulkan::LogicalDevice
{
```

Also `vulkan::LogicalDevice` has virtual functions that allow the user to configure it.

{% capture tip_content %}
The following virtual functions of `vulkan::LogicalDevice` allow further configuration of the device:

* <pre>void prepare_physical_device_features(
    vk::PhysicalDeviceFeatures& features10,
    vk::PhysicalDeviceVulkan11Features& features11,
    vk::PhysicalDeviceVulkan12Features& features12,
    vk::PhysicalDeviceVulkan13Features& features13) const</pre>
<span class="bullet_point_description">Override this function to change the default physical device features.
Each argument is a non-const reference to a vulkan class that allows changing the necessary features by
calling member functions on them. For example, `features10.setDepthClamp(true)` or `features13.setShaderZeroInitializeWorkgroupMemory(true)`.
See the vulkan documentation of {% include link_to_spec_1.3 name="VkPhysicalDeviceFeatures" %},
{% include link_to_spec_1.3 name="VkPhysicalDeviceVulkan11Features" %},
{% include link_to_spec_1.3 name="VkPhysicalDeviceVulkan12Features" %} and
{% include link_to_spec_1.3 name="VkPhysicalDeviceVulkan13Features" %} for a list of things that can be changed;
or consult <span class="command">/usr/include/vulkan/vulkan_structs.hpp</span> for the available member functions.</span>

* `void prepare_logical_device(DeviceCreateInfo& device_create_info) const`
<span class="bullet_point_description">Override this function to add QueueRequest objects.
The default will create a graphics and presentation queue. See below.</span>
{% endcapture %}{% include lv_note.html %}

&nbsp;

Every time `create_root_window` is called a `int` "cookie" must be passed.
This cookie is matched against the cookies used in the virtual
function `prepare_logical_device` of `LogicalDevice` (see above) in order
to determine which presentation queue family to use for that window (and related windows)
and therefore should be defined in the `LogicalDevice` class.

```cpp
 public:
  static constexpr int root_window_request_cookie = 1;
};
```

When overriding the default `prepare_logical_device` one might also
need a "transfer" cookie for more fine grained control over the use of
a transfer queue family.

The only requirement here is that cookies have different values.

In this example we're not using the cookie because we're not overriding `prepare_logical_device`.
The default simply creates one graphics capable queue (for the command buffer), a presentation queue
(to present the final image) and a transfer queue (to upload textures to the GPU).
The graphics and presentation queue will be combined if possible because that is more efficient
and supported by virtually all hardware anyway.
