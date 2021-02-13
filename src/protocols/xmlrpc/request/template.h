#ifndef ClassName
#error Define ClassName before including template.h
#endif
#ifndef MethodName
#error Define MethodName before including template.h
#endif

#include "protocols/xmlrpc/macros.h"
#include "protocols/xmlrpc/Request.h"
#include "utils/REMOVE_TRAILING_COMMA.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/level.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <array>

namespace xmlrpc {

class XMLRPC_CLASSNAME_CREATE(ClassName)
{
 public:
  static constexpr int s_number_of_members = 0 XMLRPC_FOREACH_MEMBER(ClassName, XMLRPC_PLUS_ONE);
  static constexpr std::array<char const*, s_number_of_members> s_xmlrpc_names = {
    XMLRPC_FOREACH_MEMBER(ClassName, XMLRPC_DECLARE_NAME)
  };

 protected:
  XMLRPC_FOREACH_MEMBER(ClassName, XMLRPC_DECLARE_MEMBER)

 public:
  // This can be used when initializing the object through serialize.
  XMLRPC_CLASSNAME_CREATE(ClassName)() = default;

  XMLRPC_CLASSNAME_CREATE(ClassName)(
    REMOVE_TRAILING_COMMA(XMLRPC_FOREACH_MEMBER(ClassName, XMLRPC_DECLARE_PARAMETER))) :
      REMOVE_TRAILING_COMMA(XMLRPC_FOREACH_MEMBER(ClassName, XMLRPC_INITIALIZER_LIST))
  {
  }

 public:
  template<class Archive>
  void serialize(Archive& ar, unsigned int const UNUSED_ARG(version))
  {
    XMLRPC_FOREACH_MEMBER(ClassName, XMLRPC_BOOST_SERIALIZE);
  }

  void print_on(std::ostream& os) const;
};

class ClassName : public XMLRPC_CLASSNAME_CREATE(ClassName), public Request, public RequestParam
{
 private:
  void write_param(std::ostream& output) const override;

 public:
  ClassName(XMLRPC_CLASSNAME_CREATE(ClassName)&& params) : XMLRPC_CLASSNAME_CREATE(ClassName){std::move(params)}
  {
    add_param(this);
  }

  ClassName(XMLRPC_CLASSNAME_CREATE(ClassName) const& params) : XMLRPC_CLASSNAME_CREATE(ClassName){params}
  {
    add_param(this);
  }

 public:
   char const* method_name() const override { return BOOST_PP_STRINGIZE(MethodName); }
};

} // namespace xmlrpc

BOOST_CLASS_IMPLEMENTATION(xmlrpc::ClassName, boost::serialization::object_serializable)
