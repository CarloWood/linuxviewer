#pragma once

#include "RequestParam.h"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/access.hpp>
#include <vector>
#include "debug.h"

namespace xmlrpc {

class Request
{
 private:
  std::string m_method_name;
  std::vector<RequestParam const*> m_params;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, unsigned int const CWDEBUG_ONLY(version))
  {
    DoutEntering(dc::notice, "xmlrpc::Request(ar, " << version << ") [m_params.size() = " << m_params.size() << "]");
    ar & m_params;
  }

 protected:
  Request(std::string method_name = std::string{}) : m_method_name(std::move(method_name)) { }

 public:
  std::string method_name() const { return m_method_name; }
  auto begin() const { return m_params.begin(); }
  auto end() const { return m_params.end(); }

 public:
  virtual ~Request() = default;

  void add_param(RequestParam const* param)
  {
    // Either provide a method_name or only add a single RequestParam
    // (which then has to override method_name() to provide the <methodName>).
    ASSERT(m_params.empty() || !m_method_name.empty());

    if (m_method_name.empty())
    {
      m_method_name = param->method_name();
      // See above.
      ASSERT(!m_method_name.empty());
    }

    m_params.push_back(param);
  }
};

} // namespace xmlrpc
