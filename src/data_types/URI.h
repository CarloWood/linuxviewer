// homer::Url v0.3.0
// MIT License
// Original project: https://github.com/homer6/url
// Unrecognizable reformatted, and changed (c) by Carlo Wood 2021.

// This class takes inspiration and some source code from
// https://github.com/chriskohlhoff/urdl/blob/master/include/urdl/url.hpp

#pragma once

#include <map>
#include <string>
#include <string_view>

//  Url is compliant with
//      https://tools.ietf.org/html/rfc3986
//      https://tools.ietf.org/html/rfc6874
//      https://tools.ietf.org/html/rfc7320
//      and adheres to https://rosettacode.org/wiki/URI_parser examples.
//
//  Url will use default ports for known schemes, if the port is not explicitly provided.
//

class URI
{
 private:
  std::string m_scheme;
  std::string m_authority;
  std::string m_user_info;
  std::string m_username;
  std::string m_password;
  std::string m_host;
  std::string m_port;
  std::string m_path;
  std::string m_query;
  std::multimap<std::string, std::string> m_query_parameters;
  std::string m_fragment;

  bool m_secure            = false;
  bool m_ipv6_host         = false;
  bool m_authority_present = false;

  std::string m_whole_uri_storage;
  size_t m_left_position  = 0;
  size_t m_right_position = 0;
  std::string_view m_parse_target;

 public:
  URI() { }
  URI(std::string_view const& data) { from_string(data); }

  void from_string(std::string_view const& data);
  void set_secure(bool secure) { m_secure = secure; }

  std::string get_scheme() const { return m_scheme; }
  std::string get_username() const { return m_username; }
  std::string get_password() const { return m_password; }
  std::string get_host() const { return m_host; }
  std::string get_query() const { return m_query; }
  std::multimap<std::string, std::string> const& get_query_parameters() const { return m_query_parameters; }
  std::string get_fragment() const { return m_fragment; }
  bool is_ipv6() const { return m_ipv6_host; }
  bool is_secure() const { return m_secure; }

  unsigned short get_port() const;
  std::string get_path() const;

  friend bool operator==(URI const& a, URI const& b);
  friend bool operator!=(URI const& a, URI const& b);
  friend bool operator<(URI const& a, URI const& b);

  std::string to_string() const;
  explicit operator std::string() const;

 protected:
  static bool unescape_path(std::string const& in, std::string& out);

  std::string_view capture_up_to(std::string_view const right_delimiter, std::string const& error_message = "");
  bool move_before(std::string_view const right_delimiter);
  bool exists_forward(std::string_view const right_delimiter);
};
