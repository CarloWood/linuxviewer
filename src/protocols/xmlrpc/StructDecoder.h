#pragma once

#include "StructDictionary.h"
#include "DecoderBase.h"
#include "utils/AIAlert.h"

namespace xmlrpc {

template<typename T>
class StructDecoder : public StructDictionary<T>, public DecoderBase<T>
{
 protected:
  ElementDecoder* create_member_decoder(std::string_view const& name) override
  {
    try
    {
      return this->m_member.create_member_decoder(this->get_member(name));
    }
    catch (UnknownMember const& except)
    {
      return &IgnoreElement::s_ignore_element;
    }
  }

 private:
  ElementDecoder* get_array_decoder() override
  {
    // The 1 bit must be set in order to skip an <array>.
    if (!(this->m_flags & 1))
      THROW_ALERT("Unexpected <array>");
    return this;
  }

  void got_data() override
  {
    // Reset the flags because we should only skip one <array><data>
    this->m_flags &= ~1;
  }

 public:
  StructDecoder(T& member, int flags) : DecoderBase<T>(member, flags) { }
};

} // namespace xmlrpc
