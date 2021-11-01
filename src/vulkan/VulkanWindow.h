#pragma once

#include "QueueReply.h"
#include "PresentationSurface.h"
#include "Swapchain.h"
#include "CurrentFrameData.h"
#include "DescriptorSetParameters.h"
#include "ImageParameters.h"
#include "BufferParameters.h"
#include "OperatingSystem.h"
#include "statefultask/Broker.h"
#include "statefultask/TaskEvent.h"
#include "xcb-task/XcbConnection.h"
#include "threadpool/Timer.h"
#include <vulkan/vulkan.hpp>
#include <memory>
#ifdef CWDEBUG
#include "cwds/tracked_intrusive_ptr.h"
#endif

namespace xcb {
class ConnectionBrokerKey;
} // namespace xcb

namespace vulkan {
class Application;
class LogicalDevice;
} // namespace vulkan

namespace linuxviewer::OS {
class Window;
} // namespace linuxviewer::OS

namespace task {

class LogicalDevice;

/**
 * The VulkanWindow task.
 *
 */
class VulkanWindow : public AIStatefulTask, public linuxviewer::OS::Window
{
 public:
  using xcb_connection_broker_type = task::Broker<task::XcbConnection>;
  using window_cookie_type = vulkan::QueueReply::window_cookies_type;

  // This event is triggered as soon as m_window is created.
  statefultask::TaskEvent m_window_created_event;

 private:
  static constexpr condition_type connection_set_up = 1;
  static constexpr condition_type frame_timer = 2;
  static constexpr condition_type logical_device_index_available = 4;

 protected:
  // Constructor
  boost::intrusive_ptr<vulkan::Application> m_application;                // Pointer to the underlaying application, which terminates when the last such reference is destructed.

 private:
  // set_title
  std::string m_title;
  // set_logical_device_task
  LogicalDevice const* m_logical_device_task = nullptr;                   // Cache valued of the task::LogicalDevice const* that was passed to
  // set_xcb_connection_broker_and_key                                    // Application::create_root_window, if any. That can be nullptr so don't use it.
  boost::intrusive_ptr<xcb_connection_broker_type> m_broker;
  xcb::ConnectionBrokerKey const* m_broker_key;

  // run
  window_cookie_type m_window_cookie = {};                                // Unique bit for the creation event of this window (as determined by the user).
  boost::intrusive_ptr<task::XcbConnection const> m_xcb_connection_task;  // Initialized in VulkanWindow_start.
  std::atomic_int m_logical_device_index = -1;                            // Index into Application::m_logical_device_list.
                                                                          // Initialized in LogicalDevice_create by call to Application::create_device.
  vulkan::PresentationSurface m_presentation_surface;                     // The presentation surface information (surface-, graphics- and presentation queue handles).
  vulkan::Swapchain m_swapchain;                                          // The swap chain used for this surface.

  threadpool::Timer::Interval m_frame_rate_interval;                      // The minimum time between two frames.
  threadpool::Timer m_frame_rate_limiter;

#ifdef CWDEBUG
  bool const mVWDebug;                                                    // A copy of mSMDebug.
#endif

 protected:
  // UNSORTED REMAINING OBJECTS.
  static constexpr vk::Format s_default_depth_format = vk::Format::eD16Unorm;
  std::vector<std::unique_ptr<vulkan::FrameResourcesData>> m_frame_resources_list;
  vulkan::CurrentFrameData m_current_frame = { nullptr, 0, 0, {} };
  vulkan::DescriptorSetParameters m_descriptor_set;
  vulkan::ImageParameters m_background_texture;
  vulkan::ImageParameters m_texture;
  vk::UniquePipelineLayout m_pipeline_layout;

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum vulkan_window_state_type {
    VulkanWindow_xcb_connection = direct_base_type::state_end,
    VulkanWindow_create,
    VulkanWindow_logical_device_index_available,
    VulkanWindow_acquire_queues,
    VulkanWindow_create_swapchain,
    VulkanWindow_render_loop,
    VulkanWindow_close
  };

 public:
  /// One beyond the largest state of this task.
  static constexpr state_type state_end = VulkanWindow_close + 1;

  /// Construct a VulkanWindow object.
  VulkanWindow(vulkan::Application* application COMMA_CWDEBUG_ONLY(bool debug = false));

  // Called from Application::create_root_window.
  void set_title(std::string&& title) { m_title = std::move(title); }
  void set_window_cookie(window_cookie_type window_cookie) { m_window_cookie = window_cookie; }
  void set_logical_device_task(LogicalDevice const* logical_device_task) { m_logical_device_task = logical_device_task; }
  void set_xcb_connection_broker_and_key(boost::intrusive_ptr<xcb_connection_broker_type> broker, xcb::ConnectionBrokerKey const* broker_key)
    // The broker_key object must have a life-time longer than the time it takes to finish task::XcbConnection.
    { m_broker = std::move(broker); m_broker_key = broker_key; }

  // Called by Application::create_device or VulkanWindow_logical_device_index_available.
  void set_logical_device_index(int index)
  {
    // Do not pass a root_window to Application::create_logical_device that was already associated with a logical device
    // by passing a logical device to Application::create_root_window that created it.
    //
    // You either call:
    //
    // auto root_window1 = application.create_root_window(std::make_unique<Window>(), {400, 400}, LogicalDevice::window_cookie1);
    // auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), root_window1);
    //
    // OR
    //
    // auto root_window2 = application.create_root_window(std::make_unique<Window>(), {400, 400}, LogicalDevice::window_cookie2, *logical_device);
    // // Do not pass root_window2 to create_logical_device.
    //
    ASSERT(m_logical_device_index.load(std::memory_order::relaxed) == -1);
    m_logical_device_index.store(index, std::memory_order::relaxed);
    signal(logical_device_index_available);
  }

  // Accessors.
  vk::SurfaceKHR vh_surface() const
  {
    return m_presentation_surface.vh_surface();
  }

  window_cookie_type window_cookie() const
  {
    return m_window_cookie;
  }

  int get_logical_device_index() const
  {
    // Bug in linuxviewer. This should never happen: set_logical_device_index()
    // should always be called before this accessor is used.
    int logical_device_index = m_logical_device_index.load(std::memory_order::relaxed);
    ASSERT(logical_device_index != -1);
    return logical_device_index;
  }

  vulkan::PresentationSurface const& presentation_surface() const
  {
    return m_presentation_surface;
  }

  vulkan::LogicalDevice* get_logical_device() const;

  vulkan::Swapchain const& swapchain() const
  {
    return m_swapchain;
  }

  void OnWindowSizeChanged(uint32_t width, uint32_t height) override final;

  void set_image_memory_barrier(
    vk::Image vh_image,
    vk::ImageSubresourceRange const& image_subresource_range,
    vk::ImageLayout current_image_layout,
    vk::AccessFlags current_image_access,
    vk::PipelineStageFlags generating_stages,
    vk::ImageLayout new_image_layout,
    vk::AccessFlags new_image_access,
    vk::PipelineStageFlags consuming_stages) const;

  void copy_data_to_image(uint32_t data_size, void const* data, vk::Image target_image,
    uint32_t width, uint32_t height, vk::ImageSubresourceRange const& image_subresource_range,
    vk::ImageLayout current_image_layout, vk::AccessFlags current_image_access, vk::PipelineStageFlags generating_stages,
    vk::ImageLayout new_image_layout, vk::AccessFlags new_image_access, vk::PipelineStageFlags consuming_stages) const;

  void copy_data_to_buffer(uint32_t data_size, void const* data, vk::Buffer target_buffer,
    vk::DeviceSize buffer_offset, vk::AccessFlags current_buffer_access, vk::PipelineStageFlags generating_stages,
    vk::AccessFlags new_buffer_access, vk::PipelineStageFlags consuming_stages) const;

 private:
  void acquire_queues();
  void create_swapchain();

 public:
  virtual threadpool::Timer::Interval get_frame_rate_interval() const;

  // Called from Application::*same name*.
  virtual void create_render_passes() = 0;
  virtual void create_graphics_pipeline() = 0;
  virtual void create_vertex_buffers() = 0;
  void create_descriptor_set();
  void create_textures();
  void create_pipeline_layout();
  void create_frame_resources();

 protected:
  virtual void draw_frame() = 0;

  virtual void OnWindowSizeChanged_Pre();
  virtual void OnWindowSizeChanged_Post();

 protected:
  void start_frame(vulkan::CurrentFrameData& current_frame);
  void finish_frame(vulkan::CurrentFrameData& current_frame, vk::CommandBuffer command_buffer, vk::RenderPass render_pass);
  vk::UniqueFramebuffer create_framebuffer(std::vector<vk::ImageView> const& image_views, vk::Extent2D const& extent, vk::RenderPass render_pass) const;
  void acquire_image(vulkan::CurrentFrameData& current_frame, vk::RenderPass render_pass);

 protected:
  /// Call finish() (or abort()), not delete.
  ~VulkanWindow() override;

  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override final;

  /// Set up task for running.
  void initialize_impl() override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;

  /// Close window.
  void finish_impl() override;
};

} // namespace task

#ifdef CWDEBUG
//DECLARE_TRACKED_BOOST_INTRUSIVE_PTR(task::VulkanWindow)
#endif
