---
layout: chapter
title: Linuxviewer API
toc_title: HelloTriangle.h
utterance_id: lv_API_HelloTriangle_h_draft
---

## {% include link_to_github folder="examples/hello_triangle/" name="HelloTriangle.h" %}

```cpp
#pragma once
```

It's a header...

<div class="extended-hr"><hr /></div>
```cpp
#include <vulkan/Application.h>
```

Needed because `HelloTriangle` is derived from `vulkan::Application`.

```cpp
class HelloTriangle : public vulkan::Application
{
 public:
  std::u8string application_name() const override
  {
    return u8"Hello Triangle";
  }
};
```

All the default virtual functions of `vulkan::Application` are good for this "hello world" application,
except that default `"Application Name"`, noone can live with that.

<a id="virtual-functions-Application" />
{% capture tip_content %}
The following virtual functions of `vulkan::Application` allow further configuration of the application:

* `std::string default_display_name() const`
<span class="bullet_point_description">The default <span class="command">DISPLAY</span> name to use (can be overridden by parse_command_line_parameters) (default: `":0"`).</span>

* `int thread_pool_number_of_worker_threads() const`
<span class="bullet_point_description">The number of worker threads to use for the thread pool (default: `vulkan::Application::default_number_of_threads` which is 8).</span>

* `int thread_pool_queue_capacity(QueuePriority priority) const`
<span class="bullet_point_description">The size of the thread pool queues (default: equal to the number of worker threads).</span>

* `int thread_pool_reserved_threads(QueuePriority priority) const`
<span class="bullet_point_description">The number of reserved threads for each queue (default: `vulkan::Application::default_reserved_threads` which is 1).</span>

* `void prepare_instance_info(vulkan::InstanceCreateInfo& instance_create_info) const`
<span class="bullet_point_description">Override this function to add Instance layers and/or extensions.</span>

* `std::u8string application_name() const`<br>
<span class="bullet_point_description">Override this function to change the default ApplicationInfo values
(default: `"Application Name"`).</span>

* `uint32_t application_version() const`<br>
<span class="bullet_point_description">Override this function to change the default application version.
The result should be a value returned by vk_utils::encode_version (default: `vk_utils::encode_version(0, 0, 0)`).</span>
{% endcapture %}{% include lv_note.html %}
