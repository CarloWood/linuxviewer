#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/macros.h"

#define xmlrpc_SunCloudMoonTextures_FOREACH_MEMBER(X) \
  X(0, UUID, sun_texture_id) \
  X(0, UUID, cloud_texture_id) \
  X(0, UUID, moon_texture_id)

class SunCloudMoonTextures
{
 private:
  xmlrpc_SunCloudMoonTextures_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_SunCloudMoonTextures_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc::ElementDecoder* get_member_decoder(members member);
};
