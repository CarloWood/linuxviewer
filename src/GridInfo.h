#pragma once

#include <string>

namespace xml {
class Bridge;
} // namespace xml

class GridInfo
{
 private:
  std::string m_gridnick;
  std::string m_uas;            // Uri
  std::string m_gridname;
  std::string m_gatekeeper;     // Uri
  std::string m_economy;        // Uri
  std::string m_platform;
  std::string m_login;          // Uri
  std::string m_welcome;        // Uri

 public:
  void xml(xml::Bridge& xml);
};
