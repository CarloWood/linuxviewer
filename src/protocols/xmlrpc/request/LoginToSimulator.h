#pragma once

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

#define ClassName LoginToSimulator
#define MethodName login_to_simulator

#include "template.h"
