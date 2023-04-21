---
layout: chapter
title: Linuxviewer API
toc_title: Introduction
utterance_id: lv_API_introduction_draft
---
{% assign drawing_a_triangle = site.baseurl | append: '/chapters/03_Drawing_a_triangle' -%}

## Introduction ##

Drawing our first triangle is the *Hello World* of Vulkan.

Linuxviewer comes with an [example application](https://github.com/CarloWood/linuxviewer/tree/master/src/examples/hello_triangle)
that opens a window with a size of 400x400 and draws a triangle in it.
<span class="command">Hello_triangle</span> is made up of the following files:

* <span class="command">HelloTriangle.cxx, HelloTriangle.h</span>:
The application definition. <span class="command">HelloTriangle.cxx</span> contains<br>`int main(int argc, char* argv[])`
and initialization that is required for every linuxviewer application. <span class="command">HelloTriangle.h</span> defines
the application class and contains more configuration by overriding a few virtual functions.

* <span class="command">Window.cxx, Window.h</span>: The window definition.
Together these define almost everything related to the Vulkan rendering; after all, the triangle that we want
to draw has to appear inside this window. We could have another window with something else, so the "triangle"
is not really related to the Application.

* <span class="command">LogicalDevice.h</span>: The definition of the
[logical device]({{ drawing_a_triangle }}/01_The_Vulkan_API.html#page_vkdevice).
The required Vulkan features and queues are specified here, again by overriding virtual functions of the base class.

* <span class="command">TrianglePipelineCharacteristic.h</span>: Most of the pipeline configuration.
The class `TrianglePipelineCharacteristic` specifies a characteristic of the pipeline. It is passed
to a pipeline factory that then creates the actual pipeline. Since we only have one characteristic
in this case I should say it specifies all characteristic.

{% capture tip_content %}
Each window in linuxviewer has its own target surface, swapchain and render loop.
Windows can run at different FPS, even when one window is a child window of another.
For example, one can create an input field that runs at 60 FPS while the rest
of the window runs at 4 FPS, still maintaining a smooth input experience.
{% endcapture %}{% include lv_note.html %}

In the next chapter we'll go over the source code of each file and add an explanation of the code.
