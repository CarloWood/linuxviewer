#pragma once

#include "Application.h"
#include "SynchronousWindow.h"

namespace vulkan {

template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
boost::intrusive_ptr<task::SynchronousWindow const> Application::create_window(
    std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
    vk::Rect2D geometry, request_cookie_type request_cookie,
    std::u8string&& title, task::LogicalDevice const* logical_device_task,
    task::SynchronousWindow const* parent_window_task)
{
  DoutEntering(dc::vulkan, "vulkan::Application::create_window<" <<
      libcwd::type_info_of<WINDOW_EVENTS>().demangled_name() << ", " <<         // First template parameter (WINDOW_EVENTS).
      libcwd::type_info_of<SYNCHRONOUS_WINDOW>().demangled_name() <<            // Second template parameter (SYNCHRONOUS_WINDOW).
      ((LibcwDoutStream << ... << (std::string(", ") + libcwd::type_info_of<SYNCHRONOUS_WINDOW_ARGS>().demangled_name())), ">(") <<
                                                                                // Tuple template parameters (SYNCHRONOUS_WINDOW_ARGS...)
      window_constructor_args << ", " << geometry << ", " << std::hex << request_cookie << std::dec << ", \"" << title << "\", " << logical_device_task << ", " << parent_window_task << ")");

  // Call Application::initialize(argc, argv) immediately after constructing the Application.
  //
  // For example:
  //
  //   MyApplication application;
  //   application.initialize(argc, argv);      // <-- this is missing if you assert here.
  //   auto root_window1 = application.create_root_window<MyWindowEvents, MyRenderLoop>({1000, 800}, MyLogicalDevice::root_window_request_cookie1);
  //
  ASSERT(m_event_loop);

  boost::intrusive_ptr<task::SynchronousWindow> window_task =
    statefultask::create_from_tuple<SYNCHRONOUS_WINDOW>(
        std::tuple_cat(
          std::move(window_constructor_args),
          std::make_tuple(this COMMA_CWDEBUG_ONLY(true))
        )
    );
  window_task->create_window_events<WINDOW_EVENTS>(geometry.extent);

  // Window initialization.
  if (title.empty())
    title = application_name();
  window_task->set_title(std::move(title));
  window_task->set_offset(geometry.offset);
  window_task->set_request_cookie(request_cookie);
  window_task->set_logical_device_task(logical_device_task);
  // The key passed to set_xcb_connection_broker_and_key MUST be canonicalized!
  m_main_display_broker_key.canonicalize();
  window_task->set_xcb_connection_broker_and_key(m_xcb_connection_broker, &m_main_display_broker_key);
  // Note that in the case of creating a child window we use the logical device of the parent.
  // However, logical_device_task can be null here because this function might be called before
  // the logical device (or parent window) was created. The SynchronousWindow task takes this
  // into account in state SynchronousWindow_create: where m_logical_device_task is null and
  // m_parent_window_task isn't, it registers with m_parent_window_task->m_logical_device_index_available_event
  // to pick up the correct value of m_logical_device_task.
  window_task->set_parent_window_task(parent_window_task);

  // Create window and start rendering loop.
  window_task->run();

  // The window is returned in order to pass it to create_logical_device.
  //
  // The pointer should be passed to create_logical_device almost immediately after
  // returning from this function with a std::move.
  return window_task;
}

} // namespace vulkan
