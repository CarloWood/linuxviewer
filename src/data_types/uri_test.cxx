#include "URI.h"

#include <iostream>
#include <string>

template <class T>
bool equal(const T& left, const T& right)
{
  if (left != right)
  {
    std::cout << "    FAIL: " << left << " != " << right << std::endl;
    return false;
  }

  std::cout << "    PASS: " << left << " == " << right << std::endl;
  return true;
};

bool test_url(std::string const& url_source, std::string const& scheme, std::string const& username,
    std::string const& password, std::string const& host, unsigned short port, std::string const& path,
    std::string const& query, std::string const& fragment, bool is_secure, bool is_ipv6)
{
  bool success = true;

  URI url{url_source};

  std::cout << std::endl << url_source << std::endl;

  if (!equal(url.get_scheme(), scheme)) success = false;
  if (!equal(url.get_username(), username)) success = false;
  if (!equal(url.get_password(), password)) success = false;
  if (!equal(url.get_host(), host)) success = false;
  if (!equal(url.get_port(), port)) success = false;
  if (!equal(url.get_path(), path)) success = false;
  if (!equal(url.get_query(), query)) success = false;
  if (!equal(url.get_fragment(), fragment)) success = false;
  if (!equal(url.is_secure(), is_secure)) success = false;
  if (!equal(url.is_ipv6(), is_ipv6)) success = false;

  if (success)
    std::cout << "PASSED" << std::endl;
  else
    std::cout << "FAILED" << std::endl;

  return success;
}

int main()
{
  int success_count = 0;
  int test_count    = 0;

  if (test_url("https://www.wikipedia.org/what-me-worry?hello=there#wonder",
        "https", "", "", "www.wikipedia.org", 443, "/what-me-worry", "hello=there", "wonder", true, false))
    ++success_count;
  ++test_count;

  if (test_url("foo://example.com:8042/over/there?name=ferret#nose",
        "foo", "", "", "example.com", 8042, "/over/there", "name=ferret", "nose", false, false))
    ++success_count;
  ++test_count;

  if (test_url("urn:example:animal:ferret:nose",
        "urn", "", "", "", 0, "example:animal:ferret:nose", "", "", false, false))
    ++success_count;
  ++test_count;

  //invalid scheme, because it contains a colon: https://tools.ietf.org/html/rfc3986#section-3.1
  //if( test_url( "jdbc:mysql://test_user:ouupppssss@localhost:3306/sakila?profileSQL=true", "jdbc:mysql", "localhost", 3306, "/sakila", "profileSQL=true", "", false, false )){ success_count++; } test_count++;

  if (test_url("ftp://ftp.is.co.za/rfc/rfc1808.txt",
        "ftp", "", "", "ftp.is.co.za", 21, "/rfc/rfc1808.txt", "", "", false, false))
    ++success_count;
  ++test_count;

  if (test_url("http://www.ietf.org/rfc/rfc2396.txt#header1",
        "http", "", "", "www.ietf.org", 80, "/rfc/rfc2396.txt", "", "header1", false, false))
    ++success_count;
  ++test_count;

  if (test_url("ldap://[2001:db8::7]/c=GB?objectClass=one&objectClass=two",
        "ldap", "", "", "2001:db8::7", 389, "/c=GB", "objectClass=one&objectClass=two", "", false, true))
    ++success_count;
  ++test_count;

  if (test_url("mailto:John.Maples@example.com",
        "mailto", "", "", "", 0, "John.Maples@example.com", "", "", false, false))
    ++success_count;
  ++test_count;

  if (test_url("news:comp.infosystems.www.servers.unix",
        "news", "", "", "", 0, "comp.infosystems.www.servers.unix", "", "", false, false))
    ++success_count;
  ++test_count;

  if (test_url("tel:+1-816-555-1212",
        "tel", "", "", "", 0, "+1-816-555-1212", "", "", false, false))
    ++success_count;
  ++test_count;

  if (test_url("telnet://192.0.2.16:80/",
        "telnet", "", "", "192.0.2.16", 80, "/", "", "", false, false))
    ++success_count;
  ++test_count;

  if (test_url("urn:oasis:names:specification:docbook:dtd:xml:4.1.2",
        "urn", "", "", "", 0, "oasis:names:specification:docbook:dtd:xml:4.1.2", "", "", false, false))
    ++success_count;
  ++test_count;

  if (test_url("ssh://alice@example.com",
        "ssh", "alice", "", "example.com", 22, "", "", "", true, false))
    ++success_count;
  ++test_count;

  if (test_url("https://bob:pass@example.com/place",
        "https", "bob", "pass", "example.com", 443, "/place", "", "", true, false))
    ++success_count;
  ++test_count;

  if (test_url(
          "http://example.com/?a=1&b=2+2&c=3&c=4&d=%65%6e%63%6F%64%65%64",
          "http", "", "", "example.com", 80, "/", "a=1&b=2+2&c=3&c=4&d=%65%6e%63%6F%64%65%64", "", false, false))
    ++success_count;
  ++test_count;

  if (test_url("postgresql://username@localhost/dbname?connect_timeout=10&application_name=myapp&ssl=true",
        "postgresql", "username", "", "localhost", 5432, "/dbname", "connect_timeout=10&application_name=myapp&ssl=true", "", true, false))
    ++success_count;
  ++test_count;

  URI const url("ssh://alice@example.com");
  if (equal(std::string("ssh://alice@example.com"), std::string(url)))
    ++success_count;
  ++test_count;

  std::cout << "\nPassed: " << success_count << " / " << test_count << std::endl;
}
