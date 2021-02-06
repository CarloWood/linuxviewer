#pragma once

#include "StructDictionary.h"
#include "DecoderBase.h"
#include "IgnoreElement.h"      // Not used here, but needed for all specializatons.
#include "utils/AIAlert.h"
#include <vector>

namespace xmlrpc {

template<typename T>
class ArrayOfStructDecoder : public StructDictionary<T>, public DecoderBase<std::vector<T>>
{
 protected:
  T* m_array_element;

 protected:
  ElementDecoder* get_member_decoder(std::string_view const& name) override
  {
    try
    {
      return m_array_element->get_member_decoder(this->get_member(name));
    }
    catch (UnknownMember const& except)
    {
      return &IgnoreElement::s_ignore_element;
    }
  }

 private:
  ElementDecoder* get_array_decoder() override
  {
    if (!(this->m_flags & 2))
      THROW_ALERT("Unexpected <array>");
    // Reset the flags because we should only skip one <array>.
    this->m_flags &= ~2;
    return this;
  }

  void got_data() override
  {
    this->m_member.emplace_back();
    m_array_element = &this->m_member.back();
  }

  ElementDecoder* get_struct_decoder() override
  {
    return this;
  }

 public:
  ArrayOfStructDecoder(std::vector<T>& member, int flags) : DecoderBase<std::vector<T>>(member, flags), m_array_element(nullptr) { }
};

} // namespace xmlrpc
