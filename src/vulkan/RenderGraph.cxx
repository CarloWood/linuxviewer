#include "sys.h"
#include "RenderGraph.h"
#include "ImageKind.h"
#include "utils/AIAlert.h"
#include "debug/vulkan_print_on.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif

// Possible configuration ways for a single render pass and a single attachment.
//
// ->write_to(A)          >> pass             ->write_to(B)           : attachment not used.
// ->write_to(A)          >> pass[-attachment]->write_to(B)           : { DONT_CARE, DONT_CARE }
// ->write_to(A)          >> pass[+attachment]->write_to(B)           : { LOAD, DONT_CARE }
// ->write_to(A)          >> pass[~attachment]->write_to(B)           : { CLEAR, DONT_CARE }
// ->write_to(A)          >> pass             ->write_to(attachment)  : { DONT_CARE, STORE }
// ->write_to(A)          >> pass[-attachment]->write_to(attachment)  : illegal; Can't bar an attachment that wasn't just written.
// ->write_to(A)          >> pass[+attachment]->write_to(attachment)  : { LOAD, STORE }
// ->write_to(A)          >> pass[~attachment]->write_to(attachment)  : { CLEAR, STORE } or shorter:
// ->write_to(A)          >> pass             ->write_to(~attachment) : { CLEAR, STORE }
// ->write_to(A)          >> pass[-attachment]->write_to(~attachment) : { CLEAR, STORE }  warning?
// ->write_to(A)          >> pass[+attachment]->write_to(~attachment) : illegal (can't have + and ~ at the same time).
// ->write_to(A)          >> pass[~attachment]->write_to(~attachment) : { CLEAR, STORE }  warning?
// ->write_to(attachment) >> pass             ->write_to(B)           : { LOAD, DONT_CARE }
// ->write_to(attachment) >> pass[-attachment]->write_to(B)           : attachment not used.
// ->write_to(attachment) >> pass[+attachment]->write_to(B)           : { LOAD, DONT_CARE }  warning?
// ->write_to(attachment) >> pass[~attachment]->write_to(B)           : illegal; makes no sense to clear an attachment that was just written to.
// ->write_to(attachment) >> pass             ->write_to(attachment)  : { LOAD, STORE }
// ->write_to(attachment) >> pass[-attachment]->write_to(attachment)  : illegal; makes no sense to write to an attachment that was just written to.
// ->write_to(attachment) >> pass[+attachment]->write_to(attachment)  : { LOAD, STORE } warning?
// ->write_to(attachment) >> pass[~attachment]->write_to(attachment)  : illegal; makes no sense to clear an attachment that was just written to.
// ->write_to(attachment) >> pass             ->write_to(~attachment) : illegal; makes no sense to clear an attachment that was just written to.
// ->write_to(attachment) >> pass[-attachment]->write_to(~attachment) : illegal; makes no sense to clear an attachment that was just written to.
// ->write_to(attachment) >> pass[+attachment]->write_to(~attachment) : illegal; makes no sense to clear an attachment that was just written to.
// ->write_to(attachment) >> pass[~attachment]->write_to(~attachment) : illegal; makes no sense to clear an attachment that was just written to.

// The simplest "triangle" render graph,
//
//   render_graph = render_pass[~depth]->write_to(output);
//
// should have to following output:
//
/*
vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass) returns VkResult VK_SUCCESS (0):
    device:                         VkDevice = 0x7f1638112b10 [LogicalDevice]
    pCreateInfo:                    const VkRenderPassCreateInfo* = 0x7f164d5dbd40:
        sType:                          VkStructureType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO (38)
        pNext:                          const void* = NULL
        flags:                          VkRenderPassCreateFlags = 0
        attachmentCount:                uint32_t = 2
        pAttachments:                   const VkAttachmentDescription* = 0x7f1639b22bf0
            pAttachments[0]:                const VkAttachmentDescription = 0x7f1639b22bf0:
                flags:                          VkAttachmentDescriptionFlags = 0
                format:                         VkFormat = VK_FORMAT_B8G8R8A8_SRGB (50)
                samples:                        VkSampleCountFlagBits = 1 (VK_SAMPLE_COUNT_1_BIT)
                loadOp:                         VkAttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR (1)
                storeOp:                        VkAttachmentStoreOp = VK_ATTACHMENT_STORE_OP_STORE (0)
                stencilLoadOp:                  VkAttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE (2)
                stencilStoreOp:                 VkAttachmentStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE (1)
                initialLayout:                  VkImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR (1000001002)
                finalLayout:                    VkImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR (1000001002)
            pAttachments[1]:                const VkAttachmentDescription = 0x7f1639b22c14:
                flags:                          VkAttachmentDescriptionFlags = 0
                format:                         VkFormat = VK_FORMAT_D16_UNORM (124)
                samples:                        VkSampleCountFlagBits = 1 (VK_SAMPLE_COUNT_1_BIT)
                loadOp:                         VkAttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR (1)
                storeOp:                        VkAttachmentStoreOp = VK_ATTACHMENT_STORE_OP_STORE (0)
                stencilLoadOp:                  VkAttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE (2)
                stencilStoreOp:                 VkAttachmentStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE (1)
                initialLayout:                  VkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL (3)
                finalLayout:                    VkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL (3)
        subpassCount:                   uint32_t = 1
        pSubpasses:                     const VkSubpassDescription* = 0x7f1639b30690
            pSubpasses[0]:                  const VkSubpassDescription = 0x7f1639b30690:
                flags:                          VkSubpassDescriptionFlags = 0
                pipelineBindPoint:              VkPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS (0)
                inputAttachmentCount:           uint32_t = 0
                pInputAttachments:              const VkAttachmentReference* = NULL
                colorAttachmentCount:           uint32_t = 1
                pColorAttachments:              const VkAttachmentReference* = 0x7f1639b24050
                    pColorAttachments[0]:           const VkAttachmentReference = 0x7f1639b24050:
                        attachment:                     uint32_t = 0
                        layout:                         VkImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL (2)
                pResolveAttachments:            const VkAttachmentReference* = NULL
                pDepthStencilAttachment:        const VkAttachmentReference* = 0x7f1639b2ea50:
                    attachment:                     uint32_t = 1
                    layout:                         VkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL (3)
                preserveAttachmentCount:        uint32_t = 0
                pPreserveAttachments:           const uint32_t* = NULL
        dependencyCount:                uint32_t = 2
        pDependencies:                  const VkSubpassDependency* = 0x7f1639b2ea60
            pDependencies[0]:               const VkSubpassDependency = 0x7f1639b2ea60:
                srcSubpass:                     uint32_t = 4294967295
                dstSubpass:                     uint32_t = 0
                srcStageMask:                   VkPipelineStageFlags = 1024 (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
                dstStageMask:                   VkPipelineStageFlags = 1024 (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
                srcAccessMask:                  VkAccessFlags = 256 (VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
                dstAccessMask:                  VkAccessFlags = 256 (VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
                dependencyFlags:                VkDependencyFlags = 1 (VK_DEPENDENCY_BY_REGION_BIT)
            pDependencies[1]:               const VkSubpassDependency = 0x7f1639b2ea7c:
                srcSubpass:                     uint32_t = 0
                dstSubpass:                     uint32_t = 4294967295
                srcStageMask:                   VkPipelineStageFlags = 1024 (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
                dstStageMask:                   VkPipelineStageFlags = 1024 (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
                srcAccessMask:                  VkAccessFlags = 256 (VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
                dstAccessMask:                  VkAccessFlags = 256 (VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
                dependencyFlags:                VkDependencyFlags = 1 (VK_DEPENDENCY_BY_REGION_BIT)
    pAllocator:                     const VkAllocationCallbacks* = NULL
    pRenderPass:                    VkRenderPass* = 0x2b424a0000000034
*/

namespace vulkan {

RenderPassStream& RenderPassStream::operator>>(RenderPassStream& rhs)
{
  Dout(dc::renderpass, m_render_pass << " >> " << rhs.m_render_pass);
  struct Compare
  {
    RenderPassAttachment const& m_attachment;
    Compare(RenderPassAttachment const& attachment) : m_attachment(attachment) { }
    bool operator()(RenderPassAttachment const& attachment) { return m_attachment.m_attachment->m_kind_and_id.get() == attachment.m_attachment->m_kind_and_id.get(); }
  };

  Dout(dc::renderpass, "Running over all output attachments of " << m_render_pass);
  for (RenderPassAttachment const& attachment : m_render_pass->m_added_output_attachments)
  {
    Dout(dc::renderpass, "Trying output attachment \"" << attachment << "\".");
    auto barred = std::find_if(rhs.m_render_pass->m_barred_input_attachments.begin(), rhs.m_render_pass->m_barred_input_attachments.end(), Compare{attachment});
    if (barred != rhs.m_render_pass->m_barred_input_attachments.end())
    {
      Dout(dc::renderpass, "Erasing " << *barred << " from m_barred_input_attachments of " << rhs.m_render_pass);
      rhs.m_render_pass->m_barred_input_attachments.erase(barred);
      continue;
    }
    rhs.m_render_pass->add_input_attachment(*attachment.m_attachment);
#if CW_DEBUG
    auto output = std::find_if(rhs.m_render_pass->m_added_output_attachments.begin(), rhs.m_render_pass->m_added_output_attachments.end(), Compare{attachment});
    if (output != rhs.m_render_pass->m_added_output_attachments.end() && output->m_clear)
      THROW_ALERT("CLEAR-ing output attachment \"[OUTPUT]\" of render pass \"[RENDERPASS]\" would overwrite \"[OUTPUT1]\" of render pass \"[PASS1]\" without reading it.",
          AIArgs("[OUTPUT]", *output)("[RENDERPASS]", rhs.m_render_pass)("[OUTPUT1]", attachment)("[PASS1]", m_render_pass));
#endif
  }
  rhs.m_prev_render_pass = this;
  return rhs;
}

RenderPassAttachment RenderPass::get_index(Attachment const* a)
{
  auto res = m_all_attachments.emplace(m_next_index, a);
  if (res.second)
  {
    Dout(dc::renderpass, "Assigned index " << m_next_index << " to attachment \"" << a << "\" of render pass \"" << this << "\".");
    ++m_next_index;
  }
  return { res.first->m_index, a };
}

RenderPassAttachment const& RenderPass::add_input_attachment(Attachment const& a)
{
  [[maybe_unused]] auto res = m_added_input_attachments.emplace(get_index(&a));
  Dout(dc::renderpass, "Added input attachment \"" << a << "\" (" << this << "/" << res.first->m_index << ").");
#if CW_DEBUG
  // Don't add an attachment twice.
  if (!res.second)
    THROW_ALERT("Input attachment \"[INPUT]\" specified twice, in render pass \"[RENDERPASS]\".", AIArgs("[INPUT]", a)("[RENDERPASS]", this));
#endif
  return *res.first;
}

RenderPassAttachment const& RenderPass::add_output_attachment(Attachment const& a)
{
  auto res = m_added_output_attachments.emplace(get_index(&a));
#if CW_DEBUG
  auto barred = m_barred_input_attachments.find(*res.first);
  if (barred != m_barred_input_attachments.end())
    THROW_ALERT("Can not specify output attachment \"[OUTPUT]\" when also marking it with [-[OUTPUT]] in render pass \"[RENDERPASS]\".", AIArgs("[OUTPUT]", a)("[RENDERPASS]", this));
  auto input = m_added_input_attachments.find(*res.first);
  if (input != m_added_input_attachments.end() && input->m_clear)
    THROW_ALERT("Attachment \"[OUTPUT]\" already specified as input. Put the CLEAR (~) in the write_to of render pass \"[RENDERPASS]\".", AIArgs("[OUTPUT]", a)("[RENDERPASS]", this));
#endif

  Dout(dc::renderpass, "Added output attachment \"" << a << "\" (" << this << "/" << res.first->m_index << ").");
  return *res.first;
}

void RenderPass::add_output_attachment(Attachment::OpClear const& a)
{
  Dout(dc::renderpass, "Need to use LOAD_OP_CLEAR for " << a);
  auto& rpa = add_output_attachment(*a.m_attachment);
  rpa.m_clear = true;
#if CW_DEBUG
  auto input = m_added_input_attachments.find(rpa);
  if (input != m_added_input_attachments.end())
    THROW_ALERT("Can't CLEAR (~) attachment \"[OUTPUT]\" that is already specified as input, in renderpass \"[RENDERPASS]\".", AIArgs("[OUTPUT]", a)("[RENDERPASS]", this));
#endif
}

void RenderPass::bar_input_attachment(Attachment const& a)
{
  [[maybe_unused]] auto res = m_barred_input_attachments.emplace(get_index(&a));
  Dout(dc::renderpass, "Bar input attachment \"" << a << "\" from render pass \"" << this << "\".");
}

RenderPassStream* RenderPass::operator->()
{
  Dout(dc::renderpass, this << "->");
  return &m_output;
}

RenderPass& RenderPass::operator[](Attachment::OpRemove a)
{
  Dout(dc::renderpass, this << "[-" << a << "]");
  // Stop attachment a from being added as input attachment.
  bar_input_attachment(*a.m_attachment);
  return *this;
}

RenderPass& RenderPass::operator[](Attachment::OpAdd const& a)
{
  Dout(dc::renderpass, this << "[" << a << "]");
  add_input_attachment(*a.m_attachment);
  return *this;
}

RenderPass& RenderPass::operator[](Attachment::OpClear const& a)
{
  Dout(dc::renderpass, this << "[~" << a << "]");
  Dout(dc::renderpass, "Need to use LOAD_OP_CLEAR for " << a);
  add_input_attachment(*a.m_attachment).m_clear = true;
  return *this;
}

void RenderGraph::operator=(RenderPassStream const& render_passes)
{
  DoutEntering(dc::vulkan, "RenderGraph::operator=()");

  // Construct a vector with all RenderPassStream's.
  for (RenderPassStream const* render_pass_stream = &render_passes; render_pass_stream; render_pass_stream = render_pass_stream->m_prev_render_pass)
    m_graph.push_front(render_pass_stream->m_render_pass);

  // Run over all render passes.
  for (RenderPass const* render_pass : m_graph)
  {
    Dout(dc::vulkan, "RenderPass: " << render_pass);
    NAMESPACE_DEBUG::Indent indent1(2);
    {
      Dout(dc::vulkan, "Input attachments:");
      NAMESPACE_DEBUG::Indent indent2(2);
      for (RenderPassAttachment const& input_attachment : render_pass->m_added_input_attachments)
      {
        Dout(dc::vulkan, render_pass << '/' << input_attachment.m_index << " - \"" << input_attachment << "\" - " <<
            *input_attachment.m_attachment->m_kind_and_id << " - " << (input_attachment.m_clear ? "LOAD_OP_CLEAR" : ""));
      }
    }
    {
      Dout(dc::vulkan, "Output attachments:");
      NAMESPACE_DEBUG::Indent indent2(2);
      for (RenderPassAttachment const& output_attachment : render_pass->m_added_output_attachments)
      {
        Dout(dc::vulkan, render_pass << '/' << output_attachment.m_index << " - \"" << output_attachment << "\" - " <<
            *output_attachment.m_attachment->m_kind_and_id << " - " << (output_attachment.m_clear ? "LOAD_OP_CLEAR" : ""));
      }
    }
    {
      Dout(dc::vulkan(!render_pass->m_barred_input_attachments.empty()), "Internal attachments:");
      NAMESPACE_DEBUG::Indent indent2(2);
      for (RenderPassAttachment const& barred_input_attachment : render_pass->m_barred_input_attachments)
      {
        Dout(dc::vulkan, render_pass << '/' << barred_input_attachment.m_index << " - \"" << barred_input_attachment << "\" - " <<
            *barred_input_attachment.m_attachment->m_kind_and_id << " - " << (barred_input_attachment.m_clear ? "LOAD_OP_CLEAR" : ""));
        // An attachment may only be in barred at this point if it is neither in input nor in output.
        ASSERT(render_pass->m_added_input_attachments.find(barred_input_attachment) == render_pass->m_added_input_attachments.end() &&
            render_pass->m_added_output_attachments.find(barred_input_attachment) == render_pass->m_added_output_attachments.end());
      }
    }
  }
}

#ifdef CWDEBUG
// Debug ostream operators.
std::ostream& operator<<(std::ostream& os, Attachment const& a) { return os << a.m_name; }
std::ostream& operator<<(std::ostream& os, Attachment::OpClear const& a) { return os << '~' << a.m_attachment->m_name; }
std::ostream& operator<<(std::ostream& os, Attachment::OpRemove const& a) { return os << '-' << a.m_attachment->m_name; }
std::ostream& operator<<(std::ostream& os, Attachment::OpAdd const& a) { return os << '+' << a.m_attachment->m_name; }

#define DECLARATION \
    [[maybe_unused]] bool illegal = false; \
    [[maybe_unused]] vulkan::RenderPass lighting("lighting"); \
    vulkan::RenderPass render_pass("render_pass"); \
    vulkan::RenderGraph render_graph; \
    Dout(dc::renderpass, "====================== Next Test ===========================");

// From the spec.
//
// VkAttachmentDescription
//
// If the attachment uses a color format, then loadOp and storeOp are used, and stencilLoadOp and stencilStoreOp are ignored.
// If the format has depth and/or stencil components, loadOp and storeOp apply only to the depth data, while stencilLoadOp and stencilStoreOp define how the stencil data is handled.
// loadOp and stencilLoadOp define the load operations that execute as part of the first subpass that uses the attachment.
// storeOp and stencilStoreOp define the store operations that execute as part of the last subpass that uses the attachment.
//
// If an attachment is not used by any subpass, then loadOp, storeOp, stencilStoreOp, and stencilLoadOp are ignored,
// and the attachment’s memory contents will not be modified by execution of a render pass instance.
//
// During a render pass instance, input/color attachments with color formats that have a component size of 8, 16, or 32 bits must
// be represented in the attachment’s format throughout the instance. Attachments with other floating- or fixed-point color formats,
// or with depth components may be represented in a format with a precision higher than the attachment format, but must be represented
// with the same range.
//
//static
void RenderGraph::testsuite()
{
  constexpr char none = 0;      // char for overloading purposes.
  constexpr int in = 1;
  constexpr int out = 2;
  constexpr int internal = 4;

  constexpr int DONT_CARE = 0;
  constexpr int LOAD = 1;
  constexpr int CLEAR = 2;
  constexpr int STORE = 3;

  vulkan::ImageKind const k1{{}};
  vulkan::ImageViewKind const v1{k1, {}};
  vulkan::Attachment const specular{v1, "specular"};
  vulkan::Attachment const output{v1, "output"};

  // Rationale
  //
  // Ignoring dynamic rendering, the values of VkAttachmentDescription::storeOp can be VK_ATTACHMENT_STORE_OP_STORE (STORE_OP_STORE)
  // or VK_ATTACHMENT_STORE_OP_DONT_CARE (STORE_OP_DONT_CARE). Both must be considered to discard any previous content of the attachment
  // written by previous render passes. If an attachment must be preserved over a renderpass then either it shouldn't mention the
  // attachment at all, or not alter it and use STORE_OP_STORE.
  //
  // If an attachment is not an output (STORE_OP_STORE), then it must use STORE_OP_DONT_CARE and the attachment did get lost
  // for subsequent render passes. Therefore,
  //
  //   pass1->write_to(A) >> pass2->write_to(B) >> pass3[+A]->write_to(output)
  //
  // must be equivalent to
  //
  //   pass1->write_to(A) >> pass2->write_to(A, B) >> pass3->write_to(output)
  //
  // where pass2 uses STORE_OP_STORE for A.
  //
  // In the following reasoning we therefore assume that the notation
  //
  //   pass2[+A]->write_to(B)  or  pass1->write_to(A) >> pass2->write_to(B)
  //
  // where A is not mentioned in the write_to of pass2, implies that A is not used
  // at all anymore after this render pass, and we can (must) use STORE_OP_DONT_CARE.
  //
  //

  {
    DECLARATION
    render_graph = render_pass->write_to(output);               // attachment not used.
    render_graph.has_with(none, specular);
  }
  {
    DECLARATION
    // Special notation for { DONT_CARE, DONT_CARE }.
    render_graph = render_pass[-specular]->write_to(output);    // { DONT_CARE, DONT_CARE }
    render_graph.has_with(internal, specular, DONT_CARE, DONT_CARE);
  }
  // Equivalent to:
  {
    DECLARATION
    render_graph = lighting->write_to(output) >> render_pass[-specular]->write_to(output);    // { DONT_CARE, DONT_CARE }
    render_graph.has_with(internal, specular, DONT_CARE, DONT_CARE);
  }
  {
    DECLARATION
    render_graph = render_pass[+specular]->write_to(output);    // { LOAD, DONT_CARE }
    render_graph.has_with(in, specular, LOAD, DONT_CARE);
  }
  {
    DECLARATION
    render_graph = render_pass[~specular]->write_to(output);    // { CLEAR, DONT_CARE }
    render_graph.has_with(internal, specular, CLEAR, DONT_CARE);
  }
  {
    DECLARATION
    render_graph = render_pass->write_to(specular);             // { DONT_CARE, STORE }
    render_graph.has_with(out, specular, DONT_CARE, STORE);
  }
  {
    DECLARATION
    try {
      render_graph = render_pass[-specular]->write_to(specular);  // AIAlert: Output attachment "specular" is barred. illegal; Can't bar an attachment that wasn't just written.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    render_graph = render_pass[+specular]->write_to(specular);  // { LOAD, STORE }
    render_graph.has_with(in|out, specular, LOAD, STORE);
  }
  {
    DECLARATION
    try {
      render_graph = render_pass[~specular]->write_to(specular);  // { CLEAR, STORE } or shorter:
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    render_graph = render_pass->write_to(~specular);            // { CLEAR, STORE }
    render_graph.has_with(out, specular, CLEAR, STORE);
  }
  {
    DECLARATION
    try {
      render_graph = render_pass[-specular]->write_to(~specular); // AIAlert: Output attachment "specular" is barred. { CLEAR, STORE }  warning?
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    try {
      render_graph = render_pass[+specular]->write_to(~specular); // illegal (can't have + and ~ at the same time).
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    try {
      render_graph = render_pass[~specular]->write_to(~specular); // { CLEAR, STORE }  warning?
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    render_graph = lighting->write_to(~specular) >> render_pass->write_to(output);                 // { LOAD, DONT_CARE }
    render_graph.has_with(in, specular, LOAD, DONT_CARE);
  }
  {
    DECLARATION
    render_graph = lighting->write_to(~specular) >> render_pass[-specular]->write_to(output);      // attachment not used.
    render_graph.has_with(none, specular);
  }
  {
    DECLARATION
    try {
      render_graph = lighting->write_to(~specular) >> render_pass[+specular]->write_to(output);    // { LOAD, DONT_CARE } - but over specified. Lets not accept this.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    try {
      render_graph = lighting->write_to(~specular) >> render_pass[~specular]->write_to(output);    // illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    render_graph = lighting->write_to(~specular) >> render_pass->write_to(specular);               // { LOAD, STORE }
    render_graph.has_with(in|out, specular, LOAD, STORE);
  }
  {
    DECLARATION
    try {
      render_graph = lighting->write_to(~specular) >> render_pass[-specular]->write_to(specular);  // illegal; makes no sense to write to an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    try {
      render_graph = lighting->write_to(~specular) >> render_pass[+specular]->write_to(specular);  // { LOAD, STORE } - but over specified. Lets not accept this.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    try {
      render_graph = lighting->write_to(~specular) >> render_pass[~specular]->write_to(specular);  // illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    try {
      render_graph = lighting->write_to(~specular) >> render_pass->write_to(~specular);            // illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    try {
      render_graph = lighting->write_to(~specular) >> render_pass[-specular]->write_to(~specular); // illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    try {
      render_graph = lighting->write_to(~specular) >> render_pass[+specular]->write_to(~specular); // illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    DECLARATION
    try {
      render_graph = lighting->write_to(~specular) >> render_pass[~specular]->write_to(~specular); // illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }

  DoutFatal(dc::fatal, "RenderGraph::testuite successful!");
}

void RenderGraph::has_with(char, Attachment const& a) const
{
  RenderPass const* render_pass = *m_graph.rbegin();

  // Attachment not used.
  for (auto const& rpa : render_pass->m_added_input_attachments)
  {
    // Attachment should not be listed in m_added_input_attachments.
    ASSERT(rpa.m_attachment->m_kind_and_id.get() != a.m_kind_and_id.get());
  }
  for (auto const& rpa : render_pass->m_added_output_attachments)
  {
    // Attachment should not be listed in m_add_output_attachments.
    ASSERT(rpa.m_attachment->m_kind_and_id.get() != a.m_kind_and_id.get());
  }
  // Attachment should not be listed in m_barred_input_attachments.
  ASSERT(render_pass->m_barred_input_attachments.empty());
}

void RenderGraph::has_with(int in_out, Attachment const& a, int load_op, int store_op) const
{
  RenderPass const* render_pass = *m_graph.rbegin();
}

#endif // CWDEBUG

} //namespace vulkan

#if !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct renderpass("RENDERPASS");
NAMESPACE_DEBUG_CHANNELS_END
#endif
