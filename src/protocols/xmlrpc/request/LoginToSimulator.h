#pragma once

#include "protocols/xmlrpc/macros.h"
#include "protocols/xmlrpc/Request.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/level.hpp>
#include <array>

#define xmlrpc_LoginToSimulator_FOREACH_MEMBER(X) \
  X(int32_t, address_size) \
  X(int32_t, agree_to_tos) \
  X(std::string, channel) \
  X(int32_t, extended_errors) \
  X(std::string, first) \
  X(std::string, host_id) \
  X(std::string, id0) \
  X(std::string, last) \
  X(int32_t, last_exec_duration) \
  X(int32_t, last_exec_event) \
  X(std::string, mac) \
  X(std::string, passwd) \
  X(std::string, platform) \
  X(std::string, platform_string) \
  X(std::string, platform_version) \
  X(int32_t, read_critical) \
  X(std::string, start) \
  X(std::string, version) \
  X(std::vector<std::string>, options)

namespace xmlrpc {

struct LoginToSimulatorCreate
{
  xmlrpc_LoginToSimulator_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, unsigned int const UNUSED_ARG(version))
  {
    xmlrpc_LoginToSimulator_FOREACH_MEMBER(XMLRPC_BOOST_SERIALIZE);
  }
};

class LoginToSimulator : public Request, public RequestParam
{
 public:
  static constexpr int s_number_of_members = XMLRPC_NUMBER_OF_MEMBERS(LoginToSimulator);
  static constexpr std::array<char const*, s_number_of_members> s_xmlrpc_names = {
    xmlrpc_LoginToSimulator_FOREACH_MEMBER(XMLRPC_DECLARE_NAME)
  };

 private:
  xmlrpc_LoginToSimulator_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

  void write_param(std::ostream& output) const override;

 public:
  LoginToSimulator(LoginToSimulatorCreate const& params) :
    RequestParam{0}
    xmlrpc_LoginToSimulator_FOREACH_MEMBER(XMLRPC_INITIALIZER_LIST_FROM_PARAMS)
  {
    add_param(this);
  }

  LoginToSimulator(unsigned int version = 1) : RequestParam{version}
  {
    add_param(this);
  }

  LoginToSimulator(
      xmlrpc_LoginToSimulator_FOREACH_MEMBER(XMLRPC_DECLARE_PARAMETERS)
      unsigned int version = 1
  ) : RequestParam{version}
      xmlrpc_LoginToSimulator_FOREACH_MEMBER(XMLRPC_INITIALIZER_LIST)
  {
    add_param(this);
  }

  template<class Archive>
  void serialize(Archive& ar, unsigned int const UNUSED_ARG(version))
  {
    xmlrpc_LoginToSimulator_FOREACH_MEMBER(XMLRPC_BOOST_SERIALIZE);
  }

 public:
   char const* method_name() const override { return "login_to_simulator"; }

   void print_on(std::ostream& os) const;
};

} // namespace xmlrpc

BOOST_CLASS_IMPLEMENTATION(xmlrpc::LoginToSimulator, boost::serialization::object_serializable)
