#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/initialize.h"
#include "evio/protocol/xmlrpc/macros.h"

#define xmlrpc_SunCloudMoonTextures_FOREACH_MEMBER(X) \
  X(UUID, sun_texture_id) \
  X(UUID, cloud_texture_id) \
  X(UUID, moon_texture_id)

class SunCloudMoonTextures
{
 private:
  xmlrpc_SunCloudMoonTextures_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_SunCloudMoonTextures_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  evio::protocol::xmlrpc::ElementDecoder* create_member_decoder(members member);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
