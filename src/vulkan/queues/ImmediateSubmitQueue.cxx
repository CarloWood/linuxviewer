#include "sys.h"
#include "ImmediateSubmitQueue.h"
#include "utils/AIAlert.h"

namespace task {

ImmediateSubmitQueue::ImmediateSubmitQueue(
    vulkan::LogicalDevice const* logical_device,
    vulkan::Queue const& queue
    COMMA_CWDEBUG_ONLY(bool debug)) :
  direct_base_type(CWDEBUG_ONLY(debug)),
  m_command_pool(logical_device, queue.queue_family()
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_command_pool"))),
  m_queue(queue)
{
  DoutEntering(dc::statefultask(mSMDebug), "ImmediateSubmitQueue::ImmediateSubmitQueue(" << logical_device << ", " << queue << ") [" << this << "]");
}

ImmediateSubmitQueue::~ImmediateSubmitQueue()
{
  DoutEntering(dc::statefultask(mSMDebug), "ImmediateSubmitQueue::~ImmediateSubmitQueue() [" << this << "]");
}

char const* ImmediateSubmitQueue::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(ImmediateSubmitQueue_need_action);
    AI_CASE_RETURN(ImmediateSubmitQueue_done);
  }
  AI_NEVER_REACHED
}

void ImmediateSubmitQueue::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case ImmediateSubmitQueue_need_action:
      flush_new_data([this](vulkan::ImmediateSubmitRequest&& submit_request){
        Dout(dc::vulkan, "ImmediateSubmitQueue_need_action: received submit_request: " << submit_request << " [" << this << "]");
        // Create a temporary command buffer.

        // Allocate a temporary command buffer from the command pool.
        vulkan::handle::CommandBuffer tmp_command_buffer = m_command_pool.allocate_buffer(
            CWDEBUG_ONLY(debug_name_prefix("ImmediateSubmitQueue_need_action::tmp_command_buffer")));

        // Accessor for the command buffer (this is a noop in Release mode, but checks that the right pool is currently locked in debug mode).
        auto tmp_command_buffer_w = tmp_command_buffer(m_command_pool);

        // Record the command buffer.
        submit_request.record_commands(tmp_command_buffer_w);

        // Submit
        {
          vk::UniqueFence fence = submit_request.logical_device()->create_fence(false
              COMMA_CWDEBUG_ONLY(mSMDebug, debug_name_prefix("ImmediateSubmitQueue_need_action::fence")));

          vk::SubmitInfo submit_info{
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = tmp_command_buffer_w
          };
          static_cast<vk::Queue>(m_queue).submit({ submit_info }, *fence);

          // FIXME: don't block.
          vk::Result res = submit_request.logical_device()->wait_for_fences({ *fence }, VK_FALSE, 1000000000);
          if (res != vk::Result::eSuccess)
            THROW_ALERTC(res, "waitForFences");
        }

        // Free the temporary command buffer again.
        m_command_pool.free_buffer(tmp_command_buffer);
      });
      if (producer_not_finished())
        break;
      set_state(ImmediateSubmitQueue_done);
      [[fallthrough]];
    case ImmediateSubmitQueue_done:
      break;
  }
}

} // namespace task
