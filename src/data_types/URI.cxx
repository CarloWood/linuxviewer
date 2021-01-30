// homer::Url v0.3.0
// MIT License
// https://github.com/homer6/url

#include "sys.h"
#include "URI.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

unsigned short URI::get_port() const
{
  if (m_port.size() > 0)
    return std::atoi(m_port.c_str());

  if (m_scheme == "https") return 443;
  if (m_scheme == "http") return 80;
  if (m_scheme == "ssh") return 22;
  if (m_scheme == "ftp") return 21;
  if (m_scheme == "mysql") return 3306;
  if (m_scheme == "mongo") return 27017;
  if (m_scheme == "mongo+srv") return 27017;
  if (m_scheme == "kafka") return 9092;
  if (m_scheme == "postgres") return 5432;
  if (m_scheme == "postgresql") return 5432;
  if (m_scheme == "redis") return 6379;
  if (m_scheme == "zookeeper") return 2181;
  if (m_scheme == "ldap") return 389;
  if (m_scheme == "ldaps") return 636;

  return 0;
}

std::string URI::get_path() const
{
  std::string tmp_path;
  unescape_path(m_path, tmp_path);
  return tmp_path;
}

std::string_view URI::capture_up_to(std::string_view const right_delimiter, std::string const& error_message)
{
  m_right_position = m_parse_target.find_first_of(right_delimiter, m_left_position);

  if (m_right_position == std::string::npos && error_message.size()) { throw std::runtime_error(error_message); }

  std::string_view captured = m_parse_target.substr(m_left_position, m_right_position - m_left_position);

  return captured;
}

bool URI::move_before(std::string_view const right_delimiter)
{
  size_t position = m_parse_target.find_first_of(right_delimiter, m_left_position);

  if (position != std::string::npos)
  {
    m_left_position = position;
    return true;
  }

  return false;
}

bool URI::exists_forward(std::string_view const right_delimiter)
{
  size_t position = m_parse_target.find_first_of(right_delimiter, m_left_position);

  if (position != std::string::npos) { return true; }

  return false;
}

// This function is called only from assign, after having updated m_whole_uri_storage.
void URI::decode()
{
  // Reset target.
  m_parse_target   = m_whole_uri_storage;
  m_left_position  = 0;
  m_right_position = 0;

  m_authority_present = false;

  // scheme
  m_scheme = capture_up_to(":", "Expected : in URI");
  std::transform(m_scheme.begin(), m_scheme.end(), m_scheme.begin(), [](std::string_view::value_type c) { return std::tolower(c); });
  m_left_position += m_scheme.size() + 1;

  // authority

  if (move_before("//"))
  {
    m_authority_present = true;
    m_left_position += 2;
  }

  if (m_authority_present)
  {
    m_authority = capture_up_to("/");

    bool path_exists = false;

    if (move_before("/")) { path_exists = true; }

    if (exists_forward("?"))
    {
      m_path = capture_up_to("?");
      move_before("?");
      m_left_position++;

      if (exists_forward("#"))
      {
        m_query = capture_up_to("#");
        move_before("#");
        m_left_position++;
        m_fragment = capture_up_to("#");
      }
      else
      {
        // No fragment.
        m_query = capture_up_to("#");
      }
    }
    else
    {
      // No query.
      if (exists_forward("#"))
      {
        m_path = capture_up_to("#");
        move_before("#");
        m_left_position++;
        m_fragment = capture_up_to("#");
      }
      else
      {
        // No fragment.
        if (path_exists) { m_path = capture_up_to("#"); }
      }
    }
  }
  else
  {
    m_path = capture_up_to("#");
  }

  // Parse authority.

  // Reset target.
  m_parse_target   = m_authority;
  m_left_position  = 0;
  m_right_position = 0;

  if (exists_forward("@"))
  {
    m_user_info = capture_up_to("@");
    move_before("@");
    m_left_position++;
  }
  else
  {
    // No user_info.
  }

  // Detect ipv6.
  if (exists_forward("["))
  {
    m_left_position++;
    m_host = capture_up_to("]", "Malformed ipv6");
    m_left_position++;
    m_ipv6_host = true;
  }
  else
  {
    if (exists_forward(":"))
    {
      m_host = capture_up_to(":");
      move_before(":");
      m_left_position++;
      m_port = capture_up_to("#");
    }
    else
    {
      // No port.
      m_host = capture_up_to(":");
    }
  }

  // Parse user_info.

  // Reset target.
  m_parse_target   = m_user_info;
  m_left_position  = 0;
  m_right_position = 0;

  if (exists_forward(":"))
  {
    m_username = capture_up_to(":");
    move_before(":");
    m_left_position++;

    m_password = capture_up_to("#");
  }
  else
  {
    // No password.
    m_username = capture_up_to(":");
  }

  // Update secure.
  if (m_scheme == "ssh" || m_scheme == "https" || m_port == "443")
  {
    m_secure = true;
  }

  if (m_scheme == "postgres" || m_scheme == "postgresql")
  {
    // Reset parse target to query.
    m_parse_target   = m_query;
    m_left_position  = 0;
    m_right_position = 0;

    if (exists_forward("ssl=true")) { m_secure = true; }
  }
}

bool URI::unescape_path(std::string const& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());

  for (std::size_t i = 0; i < in.size(); ++i)
  {
    switch (in[i])
    {
      case '%':

        if (i + 3 <= in.size())
        {
          unsigned int value = 0;

          for (std::size_t j = i + 1; j < i + 3; ++j)
          {
            switch (in[j])
            {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                value += in[j] - '0';
                break;

              case 'a':
              case 'b':
              case 'c':
              case 'd':
              case 'e':
              case 'f':
                value += in[j] - 'a' + 10;
                break;

              case 'A':
              case 'B':
              case 'C':
              case 'D':
              case 'E':
              case 'F':
                value += in[j] - 'A' + 10;
                break;

              default:
                return false;
            }

            if (j == i + 1) value <<= 4;
          }

          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }

        break;

      case '-':
      case '_':
      case '.':
      case '!':
      case '~':
      case '*':
      case '\'':
      case '(':
      case ')':
      case ':':
      case '@':
      case '&':
      case '=':
      case '+':
      case '$':
      case ',':
      case '/':
      case ';':
        out += in[i];
        break;

      default:
        if (!std::isalnum(in[i])) return false;
        out += in[i];
        break;
    }
  }

  return true;
}

bool operator==(URI const& a, URI const& b)
{
  return a.m_scheme == b.m_scheme && a.m_username == b.m_username && a.m_password == b.m_password && a.m_host == b.m_host && a.m_port == b.m_port && a.m_path == b.m_path &&
         a.m_query == b.m_query && a.m_fragment == b.m_fragment;
}

bool operator!=(URI const& a, URI const& b)
{
  return !(a == b);
}

bool operator<(URI const& a, URI const& b)
{
  if (a.m_scheme < b.m_scheme) return true;
  if (b.m_scheme < a.m_scheme) return false;

  if (a.m_username < b.m_username) return true;
  if (b.m_username < a.m_username) return false;

  if (a.m_password < b.m_password) return true;
  if (b.m_password < a.m_password) return false;

  if (a.m_host < b.m_host) return true;
  if (b.m_host < a.m_host) return false;

  if (a.m_port < b.m_port) return true;
  if (b.m_port < a.m_port) return false;

  if (a.m_path < b.m_path) return true;
  if (b.m_path < a.m_path) return false;

  if (a.m_query < b.m_query) return true;
  if (b.m_query < a.m_query) return false;

  return a.m_fragment < b.m_fragment;
}

std::string URI::to_string() const
{
  return m_whole_uri_storage;
}

URI::operator std::string() const
{
  return to_string();
}
