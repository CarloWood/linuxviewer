#include "sys.h"
#include "GridInfo.h"
#include "xml/Bridge.h"

void GridInfo::xml(xml::Bridge& xml)
{
  xml.node_name("gridinfo");
  xml.child_stream("gridnick", m_gridnick);
  xml.child_stream("uas", m_uas);
  xml.child_stream("gridname", m_gridname);
  xml.child_stream("gatekeeper", m_gatekeeper);
  xml.child_stream("economy", m_economy);
  xml.child_stream("platform", m_platform);
  xml.child_stream("login", m_login);
  xml.child_stream("welcome", m_welcome);
}
