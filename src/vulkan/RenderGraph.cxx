#include "sys.h"
#include "RenderGraph.h"
#include "ImageKind.h"
#include "utils/AIAlert.h"
#include "debug/vulkan_print_on.h"
#ifdef CWDEBUG
#include "debug/debug_ostream_operators.h"
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
  Dout(dc::renderpass, "Running over all output attachments of " << m_render_pass);
  for (RenderPassAttachment const& attachment : m_render_pass->m_added_output_attachments)
  {
    Dout(dc::renderpass, "Trying output attachment \"" << attachment << "\".");
    auto barred = RenderPass::find_by_ID(rhs.m_render_pass->m_barred_input_attachments, attachment);
    if (barred != rhs.m_render_pass->m_barred_input_attachments.end())
    {
      Dout(dc::renderpass, "Erasing " << *barred << " from m_barred_input_attachments of " << rhs.m_render_pass);
      rhs.m_render_pass->m_barred_input_attachments.erase(barred);
      continue;
    }
    rhs.m_render_pass->add_input_attachment(*attachment.m_attachment);
#if CW_DEBUG
    auto output = RenderPass::find_by_ID(rhs.m_render_pass->m_added_output_attachments, attachment);
    if (output != rhs.m_render_pass->m_added_output_attachments.end() && output->m_clear)
      THROW_ALERT("CLEAR-ing output attachment \"[OUTPUT]\" of render pass \"[RENDERPASS]\" would overwrite \"[OUTPUT1]\" of render pass \"[PASS1]\" without reading it.",
          AIArgs("[OUTPUT]", *output)("[RENDERPASS]", rhs.m_render_pass)("[OUTPUT1]", attachment)("[PASS1]", m_render_pass));
#endif
  }
  m_next_render_pass = &rhs;
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
  Dout(dc::renderpass, this << "[" << a << "]");
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
  Dout(dc::renderpass, this << "[" << a << "]");
  Dout(dc::renderpass, "Need to use LOAD_OP_CLEAR for " << a);
  add_input_attachment(*a.m_attachment).m_clear = true;
  return *this;
}

void RenderGraph::operator=(RenderPassStream const& render_passes)
{
  DoutEntering(dc::renderpass, "RenderGraph::operator=()");

  // Construct a vector with all RenderPassStream's.
  for (RenderPassStream const* render_pass_stream = &render_passes; render_pass_stream; render_pass_stream = render_pass_stream->m_prev_render_pass)
  {
    m_graph.push_front(render_pass_stream->m_render_pass);
    // Remember the last pointer; this will be the first (left-most) render pass in the stream.
    m_render_pass_list = render_pass_stream;
  }
}

void RenderGraph::create()
{
  DoutEntering(dc::renderpass, "RenderGraph::create()");

  // Run over all render passes.
  for (RenderPass const* render_pass : m_graph)
  {
    // Collect all attachments and the value of m_clear per attachment.
    std::set<RenderPassAttachment, RenderPassAttachment::CompareIndex> render_pass_attachments;

    Dout(dc::renderpass, "RenderPass: " << render_pass);
    NAMESPACE_DEBUG::Indent indent1(2);
    {
      Dout(dc::renderpass, "Input attachments:");
      NAMESPACE_DEBUG::Indent indent2(2);
      for (RenderPassAttachment const& input_attachment : render_pass->m_added_input_attachments)
      {
        Dout(dc::renderpass, render_pass << '/' << input_attachment.m_index << " - \"" << input_attachment << "\" - " <<
            *input_attachment.m_attachment->m_kind_and_id << " - " << (input_attachment.m_clear ? "LOAD_OP_CLEAR" : ""));
        auto input = render_pass_attachments.insert(input_attachment);
        if (input_attachment.m_clear)
          input.first->m_clear = true;
      }
    }
    {
      Dout(dc::renderpass, "Output attachments:");
      NAMESPACE_DEBUG::Indent indent2(2);
      for (RenderPassAttachment const& output_attachment : render_pass->m_added_output_attachments)
      {
        Dout(dc::renderpass, render_pass << '/' << output_attachment.m_index << " - \"" << output_attachment << "\" - " <<
            *output_attachment.m_attachment->m_kind_and_id << " - " << (output_attachment.m_clear ? "LOAD_OP_CLEAR" : ""));
        auto output = render_pass_attachments.insert(output_attachment);
        if (output_attachment.m_clear)
          output.first->m_clear = true;
      }
    }
    {
      Dout(dc::renderpass(!render_pass->m_barred_input_attachments.empty()), "Internal attachments:");
      NAMESPACE_DEBUG::Indent indent2(2);
      for (RenderPassAttachment const& barred_input_attachment : render_pass->m_barred_input_attachments)
      {
        Dout(dc::renderpass, render_pass << '/' << barred_input_attachment.m_index << " - \"" << barred_input_attachment << "\" - " <<
            *barred_input_attachment.m_attachment->m_kind_and_id << " - " << (barred_input_attachment.m_clear ? "LOAD_OP_CLEAR" : ""));

        // This happens for the case:
        // render_pass[-A]->write_to(B)
        //
        // Making A an "internal" attachment: neither loaded nor stored (note that A is also not the output of the previous render pass,
        // otherwise it would just be unused (not an attachment of this render pass at all)).

        // An attachment may only be in barred at this point if it is neither in input nor in output.
        ASSERT(render_pass->m_added_input_attachments.find(barred_input_attachment) == render_pass->m_added_input_attachments.end() &&
            render_pass->m_added_output_attachments.find(barred_input_attachment) == render_pass->m_added_output_attachments.end());

        auto internal = render_pass_attachments.insert(barred_input_attachment);
        if (barred_input_attachment.m_clear)
          internal.first->m_clear = true;
      }
    }

    Dout(dc::renderpass, "Render pass \"" << render_pass << "\" has " << render_pass_attachments.size() << " attachments.");

    // Generate the attachment descriptions.
    std::vector<vk::AttachmentDescription> attachment_descriptions;

    // Run over all attachments of this render pass.
    for (auto& render_pass_attachment : render_pass_attachments)
    {
      Attachment const& attachment{*render_pass_attachment.m_attachment};
      vk::Format const format = attachment.image_view_kind()->format;                   // The ImageViewKind format.
      vk::ImageAspectFlags const aspect_flags = attachment.image_view_kind()->subresource_range.aspectMask;
      bool const is_color{aspect_flags == vk::ImageAspectFlagBits::eColor};
      bool const is_depth{aspect_flags & vk::ImageAspectFlagBits::eDepth};
      bool const is_stencil{aspect_flags & vk::ImageAspectFlagBits::eStencil};

      vk::AttachmentDescription description{
        .flags = {},    // Only other option is vk::AttachmentDescriptionFlagBits::eMayAlias
                        // Spec: VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT specifies that the attachment aliases the same device memory as other attachments.
                        // If flags includes VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT, then the attachment is treated as if it shares physical memory with another attachment
                        // in the same render pass. This information limits the ability of the implementation to reorder certain operations (like layout transitions and the loadOp)
                        // such that it is not improperly reordered against other uses of the same physical memory via a different attachment.
                        // If a render pass uses multiple attachments that alias the same device memory, those attachments must each include the VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT
                        // bit in their attachment description flags. Attachments aliasing the same memory occurs in multiple ways:
                        // * Multiple attachments being assigned the same image view as part of framebuffer creation.
                        // * Attachments using distinct image views that correspond to the same image subresource of an image.
                        // * Attachments using views of distinct image subresources which are bound to overlapping memory ranges.
                        // If a set of attachments alias each other, then all except the first to be used in the render pass must use an initialLayout of VK_IMAGE_LAYOUT_UNDEFINED,
                        // since the earlier uses of the other aliases make their contents undefined. Once an alias has been used and a different alias has been used after it,
                        // the first alias must not be used in any later subpasses. However, an application can assign the same image view to multiple aliasing attachment indices,
                        // which allows that image view to be used multiple times even if other aliases are used in between.
        .format = format,
        .samples = attachment.image_kind()->samples,       // The ImageKind samples.
        .loadOp = render_pass->get_load_op(attachment),
        .storeOp = render_pass->get_store_op(attachment),
        .initialLayout = render_pass->get_initial_layout(attachment),
        .finalLayout = render_pass->get_final_layout(attachment)
      };
      // If the attachment uses a color format, then loadOp and storeOp are used, and stencilLoadOp and stencilStoreOp are ignored.
      // If the format has depth and/or stencil components, loadOp and storeOp apply only to the depth data, while stencilLoadOp and stencilStoreOp define how the stencil data is handled.
      if (is_stencil)
      {
        //FIXME: Not implemented yet.
        ASSERT(false);
        //description.setStencilLoadOp();
        //description.setStencilStoreOp();
      }

      attachment_descriptions.push_back(description);
    }

    for (auto& desc : attachment_descriptions)
    {
      Dout(dc::renderpass, desc);
    }
  }
}

vk::AttachmentLoadOp RenderPass::get_load_op(Attachment const& a) const
{
  auto input = RenderPass::find_by_ID(m_added_input_attachments, a);
  bool has_input = input != m_added_input_attachments.end();
  auto output = RenderPass::find_by_ID(m_added_output_attachments, a);
  bool has_output = output != m_added_output_attachments.end();
  bool need_clear = (has_input && input->m_clear) || (has_output && output->m_clear);

  if (need_clear)
    return vk::AttachmentLoadOp::eClear;

  if (has_input)
    return vk::AttachmentLoadOp::eLoad;

  return vk::AttachmentLoadOp::eDontCare;
}

vk::AttachmentStoreOp RenderPass::get_store_op(Attachment const& a) const
{
  auto output = RenderPass::find_by_ID(m_added_output_attachments, a);
  bool has_output = output != m_added_output_attachments.end();

  if (has_output)
    return vk::AttachmentStoreOp::eStore;

  return vk::AttachmentStoreOp::eDontCare;
}

vk::ImageLayout RenderPass::get_initial_layout(Attachment const& a) const
{
  //FIXME: implement
  return vk::ImageLayout::eUndefined;
}

vk::ImageLayout RenderPass::get_final_layout(Attachment const& a) const
{
  //FIXME: implement
  return vk::ImageLayout::eUndefined;
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
    [[maybe_unused]] vulkan::RenderPass shadow("shadow"); \
    vulkan::RenderPass render_pass("render_pass"); \
    vulkan::RenderGraph render_graph; \
    Dout(dc::renderpass, "====================== Next Test ===========================")

#define TEST(test) \
    DECLARATION; \
    Dout(dc::renderpass, "Running test: " << #test); \
    render_graph = test; \
    render_graph.create();

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
    TEST(render_pass->write_to(output));                        // attachment not used.
    render_graph.has_with(none, specular);
  }
  {
    // Special notation for { DONT_CARE, DONT_CARE }.
    TEST(render_pass[-specular]->write_to(output));             // { DONT_CARE, DONT_CARE }
    render_graph.has_with(internal, specular, DONT_CARE, DONT_CARE);
  }
  // Equivalent to:
  {
    TEST(lighting->write_to(output) >> render_pass[-specular]->write_to(output));       // { DONT_CARE, DONT_CARE }
    render_graph.has_with(internal, specular, DONT_CARE, DONT_CARE);
  }
  {
    TEST(render_pass[+specular]->write_to(output));             // { LOAD, DONT_CARE }
    render_graph.has_with(in, specular, LOAD, DONT_CARE);
  }
  {
    TEST(render_pass[~specular]->write_to(output));             // { CLEAR, DONT_CARE }
    render_graph.has_with(internal, specular, CLEAR, DONT_CARE);
  }
  {
    TEST(render_pass->write_to(specular));                      // { DONT_CARE, STORE }
    render_graph.has_with(out, specular, DONT_CARE, STORE);
  }
  {
    bool illegal = false;
    try {
      TEST(render_pass[-specular]->write_to(specular));         // AIAlert: Output attachment "specular" is barred. illegal; Can't bar an attachment that wasn't just written.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    TEST(render_pass[+specular]->write_to(specular));           // { LOAD, STORE }
    render_graph.has_with(in|out, specular, LOAD, STORE);
  }
  {
    bool illegal = false;
    try {
      TEST(render_pass[~specular]->write_to(specular));         // { CLEAR, STORE } or shorter:
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    TEST(render_pass->write_to(~specular));                     // { CLEAR, STORE }
    render_graph.has_with(out, specular, CLEAR, STORE);
  }
  {
    bool illegal = false;
    try {
      TEST(render_pass[-specular]->write_to(~specular));        // AIAlert: Output attachment "specular" is barred. { CLEAR, STORE }  warning?
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(render_pass[+specular]->write_to(~specular));        // illegal (can't have + and ~ at the same time).
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(render_pass[~specular]->write_to(~specular));        // { CLEAR, STORE }  warning?
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    TEST(lighting->write_to(~specular) >> render_pass->write_to(output));               // { LOAD, DONT_CARE }
    render_graph.has_with(in, specular, LOAD, DONT_CARE);
  }
  {
    TEST(lighting->write_to(~specular) >> render_pass[-specular]->write_to(output));    // attachment not used.
    render_graph.has_with(none, specular);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->write_to(~specular) >> render_pass[+specular]->write_to(output));  // { LOAD, DONT_CARE } - but over specified. Lets not accept this.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->write_to(~specular) >> render_pass[~specular]->write_to(output));  // illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    TEST(lighting->write_to(~specular) >> render_pass->write_to(specular));             // { LOAD, STORE }
    render_graph.has_with(in|out, specular, LOAD, STORE);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->write_to(~specular) >> render_pass[-specular]->write_to(specular));// illegal; makes no sense to write to an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->write_to(~specular) >> render_pass[+specular]->write_to(specular));// { LOAD, STORE } - but over specified. Lets not accept this.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->write_to(~specular) >> render_pass[~specular]->write_to(specular));// illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->write_to(~specular) >> render_pass->write_to(~specular));          // illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->write_to(~specular) >> render_pass[-specular]->write_to(~specular));// illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->write_to(~specular) >> render_pass[+specular]->write_to(~specular));// illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->write_to(~specular) >> render_pass[~specular]->write_to(~specular));// illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    TEST(lighting->write_to(~specular) >> render_pass->write_to(output) >> shadow[+specular]->write_to(output));
    render_graph.has_with(in, specular, LOAD, STORE);
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

void RenderGraph::has_with(int in_out, Attachment const& a, int required_load_op, int required_store_op) const
{
  // Same as RenderGraph::testsuite.
  constexpr int in = 1;
  constexpr int out = 2;
  constexpr int internal = 4;

  constexpr int DONT_CARE = 0;
  constexpr int LOAD = 1;
  constexpr int CLEAR = 2;
  constexpr int STORE = 3;

  // The render pass under test is always the last render pass.
  RenderPass const* render_pass = *m_graph.rbegin();

  // Use `none` for the zero value.
  ASSERT(in_out);
  // Only use `internal` on its own.
  ASSERT(in_out == internal || !static_cast<bool>(in_out & internal));

  auto input = RenderPass::find_by_ID(render_pass->m_added_input_attachments, a);
  auto output = RenderPass::find_by_ID(render_pass->m_added_output_attachments, a);
  bool has_input = input != render_pass->m_added_input_attachments.end() && !input->m_clear;    // An 'input' that has LOAD_OP_CLEAR isn't counted as input here.
  bool has_output = output != render_pass->m_added_output_attachments.end();
  ASSERT(has_input == static_cast<bool>(in_out & in));
  ASSERT(has_output == static_cast<bool>(in_out & out));
  ASSERT(has_input || has_output || static_cast<bool>(in_out & internal));

  auto load_op = render_pass->get_load_op(a);
  auto store_op = render_pass->get_store_op(a);

  vk::AttachmentLoadOp rq_load_op = required_load_op == LOAD ? vk::AttachmentLoadOp::eLoad : required_load_op == CLEAR ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
  vk::AttachmentStoreOp rq_store_op = required_store_op == STORE ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare;

  ASSERT(load_op == rq_load_op);
  ASSERT(store_op == rq_store_op);
}

#endif // CWDEBUG

} //namespace vulkan
