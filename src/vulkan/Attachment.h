#pragma once

#include "rendergraph/Attachment.h"
#include "ClearValue.h"

namespace vulkan {

class Attachment : public rendergraph::Attachment
{
 private:
  mutable ClearValue m_clear_value;             // Default color or depth/stencil clear value.

 public:
  using rendergraph::Attachment::Attachment;

  void set_clear_value(ClearValue clear_value) const
  {
    m_clear_value = clear_value;
  }

  ClearValue const& get_clear_value() const
  {
    return m_clear_value;
  }
};

} // namespace vulkan
