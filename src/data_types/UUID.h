#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <string_view>
#ifdef CWDEBUG
#include <iosfwd>
#endif

class UUID : public boost::uuids::uuid
{
 public:
  // Default constructor creates an uninitialized UUID object!
  UUID() { }
  // Generate from string view.
  UUID(std::string_view const& sv) : boost::uuids::uuid(boost::uuids::string_generator()(sv.begin(), sv.end())) { }

  void assign_from_string(std::string_view const& sv)
  {
    // I don't like this. It seems slow.
    new(this) boost::uuids::uuid{boost::uuids::string_generator()(sv.begin(), sv.end())};
  }

  void assign_from_xmlrpc_string(std::string_view const& uuid_data)
  {
    // UUID does not need xml unescaping, since it does not contain any of '"<>&.
    assign_from_string(uuid_data);
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
