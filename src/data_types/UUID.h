#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <iosfwd>

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

  void print_on(std::ostream& os) const
  {
    os << '{' << *static_cast<boost::uuids::uuid const*>(this) << '}';
  }
};
