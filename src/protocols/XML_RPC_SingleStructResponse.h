#pragma once

#include "XML_RPC_Response.h"
#include "xmlrpc_initialize.h"
#include "utils/AIAlert.h"
#include "utils/Dictionary.h"
#include "utils/print_using.h"
#include "utils/c_escape.h"
#include "debug.h"
#include <libcwd/buf2str.h>

class XML_RPC_IgnoreElement : public XML_RPC_Response
{
  XML_RPC_Response* get_struct() override { return this; }
  XML_RPC_Response* get_array() override { return this; }
  XML_RPC_Response* get_member(std::string_view const&) override { return this; }

#ifdef CWDEBUG
  void got_member_type(xmlrpc::data_type type, char const* struct_name) override { Dout(dc::notice, "got_member_type(" << type << ", \"" << struct_name << "\") ignored"); }
#endif
  void got_characters(std::string_view const& data) override { Dout(dc::notice, "got_characters(" << libcwd::buf2str(data.data(), data.size()) << ") ignored"); }
  void got_data() override { Dout(dc::notice, "got_data() ignored"); }

 public:
  static XML_RPC_IgnoreElement s_ignore_element;
};

template<typename T>
class XML_RPC_WrapperBase : public XML_RPC_Response
{
 protected:
  T& m_member;
  int m_flags;
#ifdef CWDEBUG
  std::string m_name;
#endif

 public:
  XML_RPC_WrapperBase(T& member, int flags COMMA_CWDEBUG_ONLY(char const* name)) : m_member(member), m_flags(flags), m_name(name) { }
};

template<typename T>
class XML_RPC_MemberWrapper : public XML_RPC_WrapperBase<T>
{
  void got_characters(std::string_view const& data) override
  {
    xmlrpc::initialize(this->m_member, data);
    Dout(dc::notice, "Initialized member 'm_" << this->m_name << "' with '" << data << "'.");
  }

 public:
  using XML_RPC_WrapperBase<T>::XML_RPC_WrapperBase;
};

template<typename T>
class XML_RPC_StructWrapperBase : public XML_RPC_WrapperBase<T>
{
 protected:
  utils::Dictionary<typename T::members, int> m_dictionary;

 protected:
  XML_RPC_Response* get_struct() override
  {
    if ((this->m_flags & 1))
      THROW_ALERT("Expected <array> before <struct>");
    return this;
  }

 private:
  XML_RPC_Response* get_array() override
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

#ifdef CWDEBUG
  void got_member_type(xmlrpc::data_type type, char const* struct_name) override
  {
  }
#endif

 public:
  XML_RPC_StructWrapperBase(T& member, int flags COMMA_CWDEBUG_ONLY(char const* name)) :
    XML_RPC_WrapperBase<T>(member, flags COMMA_CWDEBUG_ONLY(name))
  {
    for (size_t i = 0; i < T::s_number_of_members; ++i)
      m_dictionary.add(static_cast<typename T::members>(i), T::s_member2name[i]);
  }
};

class XML_RPC_UnknownMemberWrapper : public XML_RPC_Response
{
  XML_RPC_Response* get_struct() override
  {
    // Implement struct type.
    ASSERT(false);
    return nullptr;
  }

 public:
  template<typename T>
  XML_RPC_UnknownMemberWrapper(T& UNUSED_ARG(unknown_member), int UNUSED_ARG(flags) COMMA_CWDEBUG_ONLY(char const* name)) { }
};

struct Unknown
{
  enum members { };
  static constexpr size_t s_numer_of_members = 0;
};

template<>
class XML_RPC_MemberWrapper<Unknown> : public XML_RPC_UnknownMemberWrapper
{
  using XML_RPC_UnknownMemberWrapper::XML_RPC_UnknownMemberWrapper;
};

template<typename T>
class XML_RPC_StructWrapper;

template<typename T>
XML_RPC_Response* create_xmlrpc_member_wrapper(T& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  return new XML_RPC_MemberWrapper<T>{member, flags COMMA_CWDEBUG_ONLY(name)};
}

template<typename T>
class XML_RPC_ArrayWrapperBase : public XML_RPC_WrapperBase<std::vector<T>>
{
 protected:
  T* m_array_element;
  utils::Dictionary<typename T::members, int> m_dictionary;

 private:
  XML_RPC_Response* get_array() override
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

  XML_RPC_Response* get_struct() override
  {
    return this;
  }

 public:
  XML_RPC_ArrayWrapperBase(std::vector<T>& member, int flags COMMA_CWDEBUG_ONLY(char const* name)) :
    XML_RPC_WrapperBase<std::vector<T>>(member, flags COMMA_CWDEBUG_ONLY(name)), m_array_element(nullptr) { }
};

template<typename T>
class XML_RPC_ArrayWrapper : public XML_RPC_ArrayWrapperBase<T>
{
  using XML_RPC_ArrayWrapperBase<T>::XML_RPC_ArrayWrapperBase;
};

template<typename T>
XML_RPC_Response* create_xmlrpc_member_wrapper(std::vector<T>& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  return new XML_RPC_ArrayWrapper<T>{member, flags|2 COMMA_CWDEBUG_ONLY(name)};
}

class XML_RPC_SingleStructResponse : public XML_RPC_Response
{
 private:
  bool m_saw_struct;

 protected:
  XML_RPC_SingleStructResponse() : m_saw_struct(false) { }

  XML_RPC_Response* get_struct() override;

 private:
  void got_characters(std::string_view const& data) override
  {
    THROW_ALERT("Unexpected characters ([DATA]) inside <struct>", AIArgs("[DATA]", utils::print_using(data, utils::c_escape)));
  }
};
