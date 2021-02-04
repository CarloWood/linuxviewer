#pragma once

#include "Gender.h"
#include "protocols/xmlrpc/StructWrapper.h"
#include "protocols/xmlrpc/create_member_wrapper.h"
#include <string>
#include <array>

#define xmlrpc_InitialOutfit_FOREACH_ELEMENT(X) \
  X(0, std::string, folder_name) \
  X(0, Gender, gender)

#define xmlrpc_InitialOutfit_ENUMERATOR_NAME_COMMA(flags, type, el) member_##el,
#define xmlrpc_InitialOutfit_DECLARATION(flags, type, el) type m_##el;
#define xmlrpc_InitialOutfit_PLUS_ONE(flags, type, el) + 1
#define xmlrpc_InitialOutfit_NUMBER_OF_ELEMENTS(Class) static_cast<size_t>(0 xmlrpc_##Class##_FOREACH_ELEMENT(xmlrpc_InitialOutfit_PLUS_ONE))

class InitialOutfit
{
 public:
  InitialOutfit() { }

  enum members {
    xmlrpc_InitialOutfit_FOREACH_ELEMENT(xmlrpc_InitialOutfit_ENUMERATOR_NAME_COMMA)
  };

  xmlrpc_InitialOutfit_FOREACH_ELEMENT(xmlrpc_InitialOutfit_DECLARATION)

 public:
  static constexpr size_t s_number_of_members = xmlrpc_InitialOutfit_NUMBER_OF_ELEMENTS(InitialOutfit);
  static std::array<char const*, s_number_of_members> s_member2name;
};

namespace xmlrpc {

template<>
class StructWrapper<InitialOutfit> : public StructWrapperBase<InitialOutfit>
{
 protected:
  ElementDecoder* get_member(std::string_view const& name) override;

#ifdef CWDEBUG
  ElementDecoder* get_struct() override
  {
    m_struct_name = "InitialOutfit";
    return StructWrapperBase<InitialOutfit>::get_struct();
  }
#endif

  using StructWrapperBase<InitialOutfit>::StructWrapperBase;
};

template<>
inline ElementDecoder* create_member_wrapper(InitialOutfit& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  return new StructWrapper<InitialOutfit>{member, flags COMMA_CWDEBUG_ONLY(name)};
}

} // namespace xmlrpc
