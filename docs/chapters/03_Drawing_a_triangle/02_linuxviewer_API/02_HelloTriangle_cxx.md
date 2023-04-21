---
layout: chapter
title: Linuxviewer API
toc_title: HelloTriangle.cxx
utterance_id: lv_API_HelloTriangle_cxx_draft
---
{% assign drawing_a_triangle = site.baseurl | append: '/chapters/03_Drawing_a_triangle' -%}

## {% include link_to_github folder="examples/hello_triangle/" name="HelloTriangle.cxx" %}

```cpp
#include "sys.h"
```

As all git submodules use [libcwd](http://carlowood.github.io/libcwd/index.html) for debugging output,
it is required to begin *every* source file (that use libcwd macros) with this line.

This will include the <span class="command">sys.h</span> header file from the following locations, in order of priority:
first, the compiler will look for the file in the root of the project; if it doesn't exist there, it will search
the root of the build directory; and finally, if neither of those locations has the file,
it will use the [<span class="command">sys.h</span>](https://github.com/CarloWood/cwds/blob/master/sys.h) file from
[<span class="command">cwds</span>](https://github.com/CarloWood/cwds).

<div class="extended-hr"><hr /></div>
```cpp
#include "HelloTriangle.h"
#include "LogicalDevice.h"
#include "Window.h"
```

These header respectively define the Application class, the LogicalDevice and the Window that we want to open.

<div class="extended-hr"><hr /></div>
```cpp
#include "debug.h"
```

This header has to be included every time a file uses one or more <span class="command">libcwd</span> macros.
Note that it is also required when compiling the application without debug support (i.e. `Release` mode)
because in that case all those macros need to be defined as empty (or you'd get error about undefined "functions").

Since other headers likely already included a header from one of the many git submodules, or linuxviewer,
it is extremely likely that <span class="command">debug.h</span> was *already* included, but it is a good
habbit to include it anyway whenever a file uses one of the <span class="command">libcwd</span> macros.
In the case of <span class="command">HelloTriangle.cxx</span> we use `Debug`, `Dout` and `DoutFatal`.

<div class="extended-hr"><hr /></div>
```cpp
#ifdef CWDEBUG
#include <utils/debug_ostream_operators.h> // Required to print AIAlert::Error to an ostream.
#endif
```

In this case we put `#ifdef CWDEBUG ... #endif` around the `#include`
to avoid including it when we are not compiling in debug mode; in particular, when we don't have
debug output (`Dout` et al are empty).

<div class="extended-hr"><hr /></div>
```cpp
// Required header to be included below all other headers (that might include vulkan headers).
#include <vulkan/lv_inline_definitions.h>
```

The complexity of the linuxviewer headers requires the use of separate files for the definition of inline functions,
in order to avoid circular header dependencies.
The downside is that those need to be included in every compilation unit.

To make this easier, the header file <span class="command">lv_inline_definitions.h</span> can be used,
below all other includes, to automatically include the <span class="command">.inl.h</span> headers
that are required and weren't included already.

{% capture tip_content %}
Note that if you need inlined functions in a *header* then it is better to include the specific
<span class="command">.inl.h</span> header, below any other headers, required for those functions
rather than <span class="command">lv_inline_definitions.h</span>.

In all cases, these <span class="command">.inl.h</span> headers must be included *after* all
linuxviewer headers have already been included, which unfortunately means that headers,
of your own project, that need <span class="command">.inl.h</span> headers must be
included after the linuxviewer headers.
{% endcapture %}{% include lv_important.html %}

<div class="extended-hr"><hr /></div>
```cpp
int main(int argc, char* argv[])
{
  Debug(NAMESPACE_DEBUG::init());
```

The line `Debug(NAMESPACE_DEBUG::init());` is required to initialize [cwds](https://github.com/CarloWood/cwds),
(the (lib)**cwd** **s**(upport) git submodule) and through that also
[<span class="command">libcwd</span>](http://carlowood.github.io/libcwd/index.html) itself.

<div class="extended-hr"><hr /></div>
```cpp
  Dout(dc::notice, "Entering main()");
```

This prints debug output, to `std::cout` because
we didn't [change the default debug ostream](http://carlowood.github.io/libcwd/reference-manual/group__group__destination.html):

{% capture debug_output %}
NOTICE        : Entering main()
{% endcapture %}{% include debug_output.html %}

and will in fact be the very first debug output that the application print
(when using <span class="command">silent = on</span> in <span class="command">.libcwdrc</span>)
because linuxviewer uses no global variables. Global variables are evil.
All initialization will happen *after* entering main.

<div class="extended-hr"><hr /></div>
```cpp
  try
  {
    // Create the application object.
    HelloTriangle application;
```

The whole application is put inside a `try ... catch` block, including
the construction of the Application object. This is the safest way to
assure that, if there is an exception thrown that isn't caught by the
vulkan engine code, we'll catch and display it here as a last resort.

In general, irrecoverable errors are thrown as `AIAlert::Error` objects
(part of the [utils](https://github.com/CarloWood/ai-utils) git submodule)
which collect detailed information from the whole call stack involved in
the error, by being caught added to and re-thrown multiple times, if needed.

The main idea of `AIAlert::Error` exceptions is to end up in a popup
window that informs the *user* that they tried to do something that
is not possible, and give detailed information about why it is not
possible (as opposed to the usual "Something went wrong, please try again").

Therefore, normally these exceptions should be caught as part of
user actions (i.e. they click on a button) and the application as a whole
should not crash, but recover "as if" the user didn't click that button.

In the case of our HelloTriangle however, there are no user actions and
any such exception *will* be fatal, so all we can do is print the error
and terminate. In general this will be the case for all exceptions that
reach `main`.

<div class="extended-hr"><hr /></div>
```cpp
    // Initialize application using the virtual functions of FrameResourcesCount.
    application.initialize(argc, argv);
```

Here is where possible commandline arguments are processed and stored
in member variables of the application object.

Among others, this causes the virtual function
`parse_command_line_parameters(int argc, char* argv[])` to be called.
By overriding this virtual function in the user class (`HelloTriangle` in this case)
the user can add their own decoding and handling of command line parameters.

There are several other
[virtual functions]({{ page.path | remove:page.name | relative_url }}/03_HelloTriangle_h.html#virtual-functions-Application)
of `vulkan::Application`, the base call of `HelloTriangle`, that can be overridden.

<div class="extended-hr"><hr /></div>
```cpp
    // Create a window.
    auto root_window = application.create_root_window<vulkan::WindowEvents, Window>({400, 400}, LogicalDevice::root_window_request_cookie);
```

This creates a "root" window. This is not the same as the X11 root window that spans
the whole monitor, it just means that it is a normal window, as opposed to a child window (of another window).

The arguments passed are the initial size and a `int` value that is used while assigning
[command queues]({{ drawing_a_triangle }}/01_The_Vulkan_API.html#page_vkdevice)

<div class="extended-hr"><hr /></div>
```cpp
    // Create a logical device that supports presenting to root_window.
    auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), std::move(root_window));
```

This figures out which [Physical Device]({{ drawing_a_triangle }}/01_The_Vulkan_API.html#page_vkphysicaldevice)
to use, by looking for the most powerful GPU that supports the requested `LogicalDevice` (as defined in `LogicalDevice.h`), and is
able to display the requested window. It then creates and returns a smart pointer to the created
[<span class="command">VkDevice</span>]({{ drawing_a_triangle }}/01_The_Vulkan_API.html#page_vkdevice)
in the form of a `boost::intrusive_ptr<task::LogicalDevice>`; this is not really the <span class="command">VkDevice</span>,
it is an [aistatefultask](https://github.com/CarloWood/ai-statefultask) 'task' object that is will create the device,
possibly in another thread (it's complicated). Such tasks are [always](http://carlowood.github.io/statefultask/lifecycle.html)
returned as `boost::intrusive_ptr`s. Note that going out of scope means automatic destruction of the device task, and consequently,
the window because the `LogicalDevice` owns this window now (it was moved there), and the logical device.

<div class="extended-hr"><hr /></div>
```cpp
    // Run the application.
    application.run();
```

Now that everything has been "configured", really begin.
This does too much to explain here and even if it wasn't--- you don't want to know.
In fact, this opens the window and shows a triangle.

<img src="{{ '/assets/LV_HelloTriangle.png' | relative_url }}" alt="Hello Triangle" id="img_hellotriangle_large" />

<div class="extended-hr"><hr /></div>
```cpp
  }
  catch (AIAlert::Error const& error)
  {
    // Application terminated with an error.
    Dout(dc::warning, error << ", caught in HelloTriangle.cxx");
  }
#ifndef CWDEBUG // Commented out so we can see in gdb where an exception is thrown from.
  catch (std::exception& exception)
  {
    DoutFatal(dc::core, "std::exception: " << exception.what() << " caught in HelloTriangle.cxx");
  }
#endif
```

Poor mans error handling. Just print caught exceptions instead of letting them go to waste. Then exit main.
If the (last) window is closed `application.run()` runs normally and we also leave main, of course.

<div class="extended-hr"><hr /></div>
```cpp
  Dout(dc::notice, "Leaving main()");
}
```

But not before printing that we do so! Note this is not the last debug output line because linuxviewer
*does* use a few singletons that are destructed after `main` as a result of having `static` member objects.
