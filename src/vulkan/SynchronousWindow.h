#ifndef VULKAN_SYNCHRONOUS_WINDOW_H
#define VULKAN_SYNCHRONOUS_WINDOW_H

#include "SemaphoreWatcher.h"
#include "SynchronousTask.h"
#include "PresentationSurface.h"
#include "Swapchain.h"
#include "CurrentFrameData.h"
#include "OperatingSystem.h"
#include "SynchronousEngine.h"
#include "Concepts.h"
#include "ImageKind.h"
#include "SamplerKind.h"
#include "RenderPass.h"
#include "InputEvent.h"
#include "GraphicsSettings.h"
#include "Pipeline.h"
#include "queues/QueueReply.h"
#include "pipeline/Handle.h"
#include "rendergraph/RenderGraph.h"
#include "rendergraph/Attachment.h"
#include "shader_builder/SPIRVCache.h"
#include "shader_builder/shader_resource/Texture.h"
#include "ImGui.h"
#include "vk_utils/TimerData.h"
#include "statefultask/Broker.h"
#include "statefultask/TaskEvent.h"
#include "statefultask/AIEngine.h"
#include "statefultask/RunningTasksTracker.h"
#include "statefultask/TaskCounterGate.h"
#include "xcb-task/XcbConnection.h"
#include "threadpool/Timer.h"
#include "utils/Badge.h"
#include "utils/UniqueID.h"
#include "FrameResourceIndex.h"
#include <vulkan/vulkan.hpp>
#include <memory>
#ifdef CWDEBUG
#include "cwds/tracked_intrusive_ptr.h"
#endif
#ifdef TRACY_ENABLE
#include "TracyC.h"
#include "utils/Array.h"
#endif

namespace xcb {
class ConnectionBrokerKey;
} // namespace xcb

namespace task {
class LogicalDevice;
class SynchronousTask;
class SynchronousWindow;
namespace synchronous {
class MoveNewPipelines;
} // namespace synchronous
} // namespace task

namespace vulkan {
class Application;
class LogicalDevice;
class Ambifix;
class AmbifixOwner;
class Swapchain;
class WindowEvents;

namespace shader_builder {
namespace shader_resource { }
class SPIRVCache;
} // shader_builder
namespace shader_resource = shader_builder::shader_resource;

namespace pipeline {
class FactoryHandle;
} // namespace pipeline

namespace detail {

class DelaySemaphoreDestruction
{
 private:
  int pos = 0;
  std::array<std::vector<vk::UniqueSemaphore>, 16> m_queue;

 public:
  void add(utils::Badge<Swapchain>, vk::UniqueSemaphore&& semaphore, int delay)
  {
    m_queue[(pos + delay) % m_queue.size()].emplace_back(std::move(semaphore));
  }

  void step(utils::Badge<task::SynchronousWindow>)
  {
    pos = (pos + 1) % m_queue.size();
    if (!m_queue[pos].empty())
      m_queue[pos].clear();
  }
};

} // namespace detail
} // namespace vulkan

namespace linuxviewer::OS {
class Window;
} // namespace linuxviewer::OS

namespace task {

/**
 * The SynchronousWindow task.
 *
 */
class SynchronousWindow : public AIStatefulTask, protected vulkan::SynchronousEngine
{
 public:
  using request_cookie_type = vulkan::QueueRequest::cookies_type;
  using xcb_connection_broker_type = Broker<XcbConnection>;
  using AttachmentIndex = vulkan::rendergraph::AttachmentIndex;

  // This event is triggered as soon as m_window is created.
  statefultask::TaskEvent m_window_created_event;

  static vulkan::ImageKind const s_depth_image_kind;
  static vulkan::ImageViewKind const s_depth_image_view_kind;
  static vulkan::ImageKind const s_color_image_kind;
  static vulkan::ImageViewKind const s_color_image_view_kind;

 private:
  static constexpr condition_type connection_set_up = 1;
  static constexpr condition_type frame_timer = 2;
  static constexpr condition_type logical_device_index_available = 4;
  static constexpr condition_type imgui_font_texture_ready = 8;
  static constexpr condition_type parent_window_created = 16;
  static constexpr condition_type condition_pipeline_available = 32;

 protected:
  // Constructor
  boost::intrusive_ptr<vulkan::Application> m_application;                // Pointer to the underlying application, which terminates when the last such reference is destructed.
  vulkan::LogicalDevice* m_logical_device = nullptr;                      // Cached pointer to the LogicalDevice; set during task state LogicalDevice_create or
                                                                          // SynchronousWindow_logical_device_index_available.
 private:
  // set_title
  std::u8string m_title;                                                  // UTF8 encoded window title.
  // set_offset
  vk::Offset2D m_offset;                                                  // Initial position of the top-left corner of the window, relative to the parent window.
  // set_logical_device_task
  LogicalDevice const* m_logical_device_task = nullptr;                   // Cache valued of the task::LogicalDevice const* that was passed to
                                                                          // Application::create_window, if any. That can be nullptr so don't use it.

  // This must come *before* m_window_events in order to get the corect order of destruction (destroy window events first).
  boost::intrusive_ptr<SynchronousWindow const> m_parent_window_task;     // A pointer to the parent window, or nullptr when this is a root window.
                                                                          // It is NOT thread-safe to access the parent window without knowing what you are doing.
  // create_window_events
  std::unique_ptr<vulkan::WindowEvents> m_window_events;                  // Pointer to the asynchronous object `WindowEvents`.

  // set_xcb_connection_broker_and_key
  boost::intrusive_ptr<xcb_connection_broker_type> m_broker;
  xcb::ConnectionBrokerKey const* m_broker_key;

  // run
  request_cookie_type m_request_cookie = {};                              // Unique bit for the creation event of this window (as determined by the user).
  boost::intrusive_ptr<XcbConnection const> m_xcb_connection_task;        // Initialized in SynchronousWindow_start.
  std::atomic_int m_logical_device_index = -1;                            // Index into Application::m_logical_device_list.
                                                                          // Initialized in LogicalDevice_create by call to Application::create_device.
  vulkan::PresentationSurface m_presentation_surface;                     // The presentation surface information (surface-, graphics- and presentation queue handles).
  vulkan::Swapchain m_swapchain;                                          // The swap chain used for this surface.

  threadpool::Timer::Interval m_frame_rate_interval;                      // The minimum time between two frames.
  threadpool::Timer m_frame_rate_limiter;

  boost::intrusive_ptr<task::SemaphoreWatcher<task::SynchronousTask>> m_semaphore_watcher;  // Synchronous task that polls timeline semaphores.

  bool m_use_imgui = false;

 private:
  static constexpr int s_input_event_buffer_size = 32;                    // If the application is lagging more than 32 events behind then
                                                                          // the user is having other problems then losing key strokes.
  vulkan::InputEventBuffer m_input_event_buffer;                          // Thread-safe ringbuffer to transfer input events from EventThread to this task.
  bool m_in_focus;                                                        // Cache value of decoded input events.
#ifdef TRACY_ENABLE
 protected:
  bool m_is_tracy_window;                                                 // Set upon entering this window with the mouse; unset when a different window is entered.
  utils::Vector<TracyCZoneCtx, vulkan::SwapchainIndex> tracy_acquired_image_tracy_context;
  utils::Vector<bool, vulkan::SwapchainIndex> tracy_acquired_image_busy;
 private:
#endif

  using child_window_list_container_t = std::vector<SynchronousWindow*>;
  using child_window_list_t = aithreadsafe::Wrapper<child_window_list_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  mutable child_window_list_t m_child_window_list;                        // List with child windows.

  vulkan::GraphicsSettingsPOD m_graphics_settings;                        // Cached copy of global graphics settings; should be synchronized at the start of the render loop.

#ifdef CWDEBUG
  bool const mVWDebug;                                                    // A copy of mSMDebug.
#endif

  // Needs access to the protected SynchronousEngine base class.
  friend class SynchronousTask;

 protected:
  // Only provide read access to derive class, because m_graphics_settings is only a local cache and is overwritten when Application::m_graphics_settings is changed.
  vulkan::GraphicsSettingsPOD const& graphics_settings() const { return m_graphics_settings; }

  // Optionally called from something like a Graphics Settings window.
  void change_number_of_swapchain_images(uint32_t image_count);

 public:
  // Accessed by vulkan::rendergraph::Attachment::assign_unique_index().
  utils::UniqueIDContext<AttachmentIndex> attachment_index_context;       // Provides an unique index for registered attachments (through register_attachment).

  // Accessed by Swapchain.
  vulkan::detail::DelaySemaphoreDestruction m_delay_by_completed_draw_frames;

  statefultask::TaskEvent m_logical_device_index_available_event;         // Triggered when m_logical_device_index is set.

  // Accessed by tasks that depend on objects of this class (or derived classes).
  statefultask::RunningTasksTracker m_dependent_tasks;                    // Tasks that should be aborted before this window is destructed.
  statefultask::TaskCounterGate m_task_counter_gate;                      // Number of running task that we should wait for before this window is destructed.

  void close() { set_must_close(); }

 protected:
  static constexpr vk::Format s_default_depth_format = vk::Format::eD16Unorm;
  static constexpr vulkan::FrameResourceIndex s_default_max_number_of_frame_resources{2};       // Default size of m_frame_resources_list.
  static constexpr vulkan::SwapchainIndex s_default_max_number_of_swapchain_images{3};          // The default number of maximum number of swapchain images
                                                                                                // that the application has to take into account (for example,
                                                                                                // used for static creation of Tracy GPU zone labels).
  // Render graph nodes.
  using RenderPass = vulkan::RenderPass;                                                // Use to define render passes in derived Window class.
  using Attachment = vulkan::rendergraph::Attachment;                                   // Use to define attachments in derived Window class.
  // During construction of derived class (that must construct the needed RenderPass and Attachment objects as members).
  std::vector<RenderPass*> m_render_passes;                                             // All render pass objects.
  utils::Vector<Attachment const*> m_attachments;                                       // All known attachments, except the swapchain attachment (if any).
  vulkan::rendergraph::RenderGraph m_render_graph;                                      // Must be assigned in the derived Window class.
  RenderPass imgui_pass{this, "imgui_pass"};                                            // Must be constructed AFTER m_render_passes!

 public:
  void register_render_pass(RenderPass*);
  void unregister_render_pass(RenderPass*);
  void register_attachment(Attachment const*);
  size_t number_of_registered_attachments() const { return m_attachments.size(); }
#ifdef CWDEBUG
  auto attachments_begin() const { return m_attachments.begin(); }
  auto attachments_end() const { return m_attachments.end(); }
#endif

 protected:
  utils::Vector<std::unique_ptr<vulkan::FrameResourcesData>, vulkan::FrameResourceIndex> m_frame_resources_list;        // Vector with frame resources.
  vulkan::CurrentFrameData m_current_frame = { nullptr, vulkan::FrameResourceIndex{0}, vulkan::FrameResourceIndex{0} };

  // Initialized by create_imgui. Deinitialized by destruction.
  vk_utils::TimerData m_timer;
  vulkan::ImGui m_imgui;                // ImGui framework.

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum vulkan_window_state_type {
    SynchronousWindow_xcb_connection = direct_base_type::state_end,
    SynchronousWindow_create_child,
    SynchronousWindow_create,
    SynchronousWindow_parents_logical_device_index_available,
    SynchronousWindow_logical_device_index_available,
    SynchronousWindow_acquire_queues,
    SynchronousWindow_initialize_vulkan,
    SynchronousWindow_imgui_font_texture_ready,
    SynchronousWindow_render_loop,
    SynchronousWindow_close
  };

  /// One beyond the largest state of this task.
  static constexpr state_type state_end = SynchronousWindow_close + 1;

 public:
  /// Construct a synchronous SynchronousWindow task.
  SynchronousWindow(vulkan::Application* application COMMA_CWDEBUG_ONLY(bool debug = false));

  // Called by the constructor of vulkan::rendergraph::RenderPass objects,
  // which should be members of the derived class.
  vulkan::rendergraph::RenderGraph& render_graph() { return m_render_graph; }

  // Called from Application::create_window.
  template<vulkan::ConceptWindowEvents WINDOW_EVENTS> void create_window_events(vk::Extent2D extent);
  void set_title(std::u8string&& title) { m_title = std::move(title); }
  void set_offset(vk::Offset2D offset) { m_offset = offset; }
  void set_request_cookie(request_cookie_type request_cookie) { m_request_cookie = request_cookie; }
  void set_logical_device_task(LogicalDevice const* logical_device_task) { m_logical_device_task = logical_device_task; }
  void set_xcb_connection_broker_and_key(boost::intrusive_ptr<xcb_connection_broker_type> broker, xcb::ConnectionBrokerKey const* broker_key)
    // The broker_key object must have a life-time longer than the time it takes to finish task::XcbConnection.
    { m_broker = std::move(broker); m_broker_key = broker_key; }
  void set_parent_window_task(SynchronousWindow const* parent_window_task)
  {
    // set_parent_window_task should only be called once (from Application::create_window).
    ASSERT(!m_parent_window_task);
    // Child windows should be destructed before the parent window.
    m_parent_window_task = parent_window_task;
    // This function is always called, also when there isn't a parent window.
    if (m_parent_window_task)
      m_parent_window_task->add_child_window_task(this);
  }
#ifdef TRACY_ENABLE
  void set_is_tracy_window(bool active)
  {
    DoutEntering(dc::notice, "SynchronousWindow::set_is_tracy_window(" << std::boolalpha << active << ") [" << this << "]");
    m_is_tracy_window = active;
  }
#endif

  // The const on this method means that it is a thread-safe function. It still alters m_child_window_list.
  void add_child_window_task(SynchronousWindow* child_window_task) const
  {
    child_window_list_t::wat child_window_list_w(m_child_window_list);
    child_window_list_w->push_back(child_window_task);
  }

  // The const on this method means that it is a thread-safe function. It still alters m_child_window_list.
  void remove_child_window_task(SynchronousWindow* child_window_task) const
  {
    child_window_list_t::wat child_window_list_w(m_child_window_list);
    auto child_window = std::find(child_window_list_w->begin(), child_window_list_w->end(), child_window_task);
    // Bug in this library: a child window should ALWAYS call add_child_window_task/remove_child_window_task in pairs and in that order.
    ASSERT(child_window != child_window_list_w->end());
    child_window_list_w->erase(child_window);
  }

  // Called by Application::create_device or SynchronousWindow_logical_device_index_available.
  void set_logical_device_index(int index)
  {
    // Old comment for this ASSERT:
    //
    //  Do not pass a root_window to Application::create_logical_device that was already associated with a logical device
    //  by passing a logical device to Application::create_root_window that created it.
    //
    //  You either call:
    //
    //  auto root_window1 = application.create_root_window<WindowEvents, Window>({400, 400}, LogicalDevice::request_cookie1);
    //  auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), std::move(root_window1));
    //
    //  OR
    //
    //  application.create_root_window<WindowEvents, Window>({400, 400}, LogicalDevice::request_cookie2, *logical_device);
    //
    // It should no longer be possible for this assert to fire since the last call doesn't return an intrusive pointer anymore.
    ASSERT(m_logical_device_index.load(std::memory_order::relaxed) == -1);

    m_logical_device_index.store(index, std::memory_order::relaxed);
    signal(logical_device_index_available);
    m_logical_device_index_available_event.trigger();
  }

  // Create and run a new window task that will run a child window of this window.
  template<vulkan::ConceptWindowEvents WINDOW_EVENTS, vulkan::ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
  void create_child_window(
      std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
      vk::Rect2D geometry,
      task::SynchronousWindow::request_cookie_type request_cookie,
      std::u8string&& title = {}) const;

  // Called from LogicalDevice_create.
  void cache_logical_device() { m_logical_device = get_logical_device(); }

  // Accessors.
  vk::SurfaceKHR vh_surface() const
  {
    return m_presentation_surface.vh_surface();
  }

  vulkan::Application const& application() const
  {
    return *m_application;
  }

  vulkan::WindowEvents const* window_events() const
  {
    return m_window_events.get();
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

  // Return a cached value of get_logical_device().
  vulkan::LogicalDevice const* logical_device() const
  {
    // Bug in linuxviewer. This should not be called before m_logical_device is initialized.
    ASSERT(m_logical_device);
    return m_logical_device;
  }

  vulkan::Swapchain& swapchain() { return m_swapchain; }
  vulkan::Swapchain const& swapchain() const { return m_swapchain; }
  void no_swapchain(utils::Badge<vulkan::Swapchain>) const { vulkan::SynchronousEngine::no_swapchain(); }
  void have_swapchain(utils::Badge<vulkan::Swapchain>) const { vulkan::SynchronousEngine::have_swapchain(); }

  void wait_for_all_fences() const;

  // Call this from the render loop every time that extent_changed(atomic_flags()) returns true.
  // Call only synchronously.
  vk::Extent2D get_extent() const;

  vk::RenderPass vh_imgui_render_pass() const { return imgui_pass.vh_render_pass(); }

  void handle_window_size_changed();
  bool handle_map_changed(int map_flags);
  void copy_graphics_settings();
  void add_synchronous_task(std::function<void(SynchronousWindow*)> lambda);

  void set_image_memory_barrier(
    vulkan::ResourceState const& source,
    vulkan::ResourceState const& destination,
    vk::Image vh_image,
    vk::ImageSubresourceRange const& image_subresource_range) const;

  vulkan::shader_builder::shader_resource::Texture upload_texture(
      char const* glsl_id_full_postfix, std::unique_ptr<vulkan::DataFeeder> texture_data_feeder, vk::Extent2D extent,
      int binding, vulkan::ImageViewKind const& image_view_kind, vulkan::SamplerKind const& sampler_kind, vk::DescriptorSet vh_descriptor_set,
      AIStatefulTask::condition_type texture_ready);

  void detect_if_imgui_is_used();

 public:
  // The same type as the type defined in vulkan::pipeline::FactoryHandle, vulkan::pipeline::Handle::PipelineFactoryIndex,
  // task::PipelineFactory, task::SynchronousWindow and vulkan::Application with the same name.
  using PipelineFactoryIndex = vulkan::pipeline::Handle::PipelineFactoryIndex;

 protected:
  utils::Vector<boost::intrusive_ptr<task::PipelineFactory>> m_pipeline_factories;
  utils::Vector<utils::Vector<vk::UniquePipeline, vulkan::pipeline::Index>, PipelineFactoryIndex> m_pipelines;
//  std::map<vulkan::FlatPipelineLayout, vk::UniquePipelineLayout> m_pipeline_layouts;

  // Called from create_graphics_pipelines of derived class.
  vulkan::pipeline::FactoryHandle create_pipeline_factory(vulkan::Pipeline& pipeline_out, vk::RenderPass vh_render_pass COMMA_CWDEBUG_ONLY(bool debug));

  // Return the vulkan handle of this pipeline.
  vk::Pipeline vh_graphics_pipeline(vulkan::pipeline::Handle pipeline_handle) const;

 public:
  void have_new_pipeline(vulkan::Pipeline&& pipeline_handle_and_layout, vk::UniquePipeline&& pipeline);

  // Called by state MoveNewPipelines_done.
  void pipeline_factory_done(utils::Badge<synchronous::MoveNewPipelines>, PipelineFactoryIndex index);

  // Called by vulkan::pipeline::FactoryHandle::generate.
  inline task::PipelineFactory* pipeline_factory(PipelineFactoryIndex factory_index) const;

 private:
  // SynchronousWindow_acquire_queues:
  void acquire_queues();

  // SynchronousWindow_initialize_vukan:
  // (virtual functions are implemented by most derived class)
  virtual void set_default_clear_values(vulkan::rendergraph::ClearValue& color, vulkan::rendergraph::ClearValue& depth_stencil);
  void prepare_swapchain();
  virtual void create_render_graph() = 0;
  void create_swapchain_images();
  void create_frame_resources();
  void create_imageless_framebuffers();
  virtual void register_shader_templates() = 0;
  virtual void create_textures() = 0;
  virtual void create_graphics_pipelines() = 0;
  void create_imgui();

  // SynchronousWindow_render_loop:
  void consume_input_events();
  virtual void draw_imgui() { }
  virtual void render_frame() = 0;

  // Called by create_imageless_framebuffers and handle_window_size_changed.
  void recreate_framebuffers(vk::Extent2D extent, uint32_t layers);
  // Called by create_imageless_framebuffers.
  void prepare_begin_info_chains();

  // Optionally overridden by derived class.

  // Called by initialize_impl():
  virtual threadpool::Timer::Interval get_frame_rate_interval() const;
  // Called by handle_window_size_changed():
  virtual void on_window_size_changed_pre();
  // Called by create_frame_resources() and handle_window_size_changed():
  virtual void on_window_size_changed_post();

 public:
  // Called by create_frame_resources() (and PresentationSurface::set_queues when TRACY_ENABLE).
  virtual vulkan::FrameResourceIndex max_number_of_frame_resources() const;

  // Called by ... when TRACY_ENABLE.
  virtual vulkan::SwapchainIndex max_number_of_swapchain_images() const;

  // Override this function to give a Window its own (or shared) pipeline cache ID.
  // Windows with the same pipeline_cache_name will share the same cache file.
  virtual std::u8string pipeline_cache_name() const;

  //FIXME: this shouldn't be a virtual function here - uniform buffers should be automatically created.
//  virtual void create_uniform_buffers(vulkan::pipeline::ShaderInputData const& shader_input_data, vulkan::descriptor::SetBindingMap const& set_binding_map);

 protected:
  void start_frame();
  void wait_command_buffer_completed();
  void submit(vulkan::handle::CommandBuffer command_buffer);
  void finish_frame();
  void acquire_image();

 public:
#ifdef CWDEBUG
  vulkan::AmbifixOwner debug_name_prefix(std::string prefix) const;
#endif

 protected:
  /// Call finish() (or abort()), not delete.
  ~SynchronousWindow() override;

  // Implementation of virtual functions of AIStatefulTask.
  char const* condition_str_impl(condition_type condition) const override;
  char const* state_str_impl(state_type run_state) const override final;
  char const* task_name_impl() const override;

  /// Set up task for running.
  void initialize_impl() override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;

  /// Close window.
  void finish_impl() override;
};

#ifdef CWDEBUG
// Make sure to print SynchronousWindow tasks by their AIStatefulTask* - so that we can see who is who also in statefultask debug output.
inline std::ostream& operator<<(std::ostream& os, SynchronousWindow const* ptr)
{
  return os << static_cast<AIStatefulTask const*>(ptr);
}
#endif

} // namespace task

#endif // VULKAN_SYNCHRONOUS_WINDOW_H

#ifndef VULKAN_PIPELINE_FACTORY_HANDLE_H
#include "pipeline/FactoryHandle.h"
#endif
#ifndef VULKAN_APPLICATION_H
#include "Application.h"
#endif
#ifndef VULKAN_WINDOW_EVENTS_H
#include "WindowEvents.h"
#endif

#ifndef VULKAN_SYNCHRONOUS_WINDOW_H_definitions
#define VULKAN_SYNCHRONOUS_WINDOW_H_definitions

namespace task {

template<vulkan::ConceptWindowEvents WINDOW_EVENTS>
void SynchronousWindow::create_window_events(vk::Extent2D extent)
{
  DoutEntering(dc::notice, "SynchronousWindow::create_window_events(" << extent << ") [" << this << " : \"" << m_title << "\"]");
  m_window_events = std::make_unique<WINDOW_EVENTS>();
  Dout(dc::notice, "m_window_events = " << m_window_events.get());
  m_window_events->set_special_circumstances(this);
  m_window_events->set_extent(extent);
}

template<vulkan::ConceptWindowEvents WINDOW_EVENTS, vulkan::ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
void SynchronousWindow::create_child_window(
    std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
    vk::Rect2D geometry,
    task::SynchronousWindow::request_cookie_type request_cookie,
    std::u8string&& title) const
{
  auto child_window = m_application->create_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(
      std::move(window_constructor_args), geometry, request_cookie, std::move(title),
      m_logical_device_task, this);
  // idem
}

//inline
task::PipelineFactory* SynchronousWindow::pipeline_factory(PipelineFactoryIndex factory_index) const
{
  return m_pipeline_factories[factory_index].get();
}

} // namespace task

#endif // VULKAN_SYNCHRONOUS_WINDOW_H_definitions
