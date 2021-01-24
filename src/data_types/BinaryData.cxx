#include "sys.h"
#include "BinaryData.h"
#include "utils/AIAlert.h"
#include "debug.h"
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/escape.hpp>
#include <iostream>

using namespace boost::archive::iterators;

namespace {

// Convert 0 --> 0, 1 --> 2 and 2 --> 1 by swapping the last two bits.
unsigned int required_padding(int n)
{
  return ((n << 1) | (n >> 1)) & 3;
}

template<class Base>
class convert : public escape<convert<Base>, Base>
{
  friend class boost::iterator_core_access;

  using super_t = escape<convert<Base>, Base>;

 public:
  char fill(char const*& bstart, char const*& bend);
  wchar_t fill(wchar_t const*& bstart, wchar_t const*& bend);

  template<class T>
  convert(T start) : super_t(Base(static_cast<T>(start))) { }
  // intel 7.1 doesn't like default copy constructor
  convert(convert const& rhs) : super_t(rhs.base_reference()) { }
};

template<class Base>
char convert<Base>::fill(char const*& bstart, char const*& bend)
{
  char current_value = *this->base_reference();
  if (current_value == '=')
    current_value = 'A';
  return current_value;
}

template<class Base>
wchar_t convert<Base>::fill(wchar_t const*& bstart, wchar_t const*& bend)
{
  wchar_t current_value = *this->base_reference();
  if (current_value == '=')
    current_value = 'A';
  return current_value;
}

char nibble(int val)
{
  return (val < 10 ? '0' : 'a' - 10) + val;
}

} // namespace

// Decode
void BinaryData::assign_from_base64(std::string_view const& data)
{
  if (data.size() % 4 != 0)
    THROW_FALERT("Input length is not a multiple of four");
  int padding_len = data.size() - 1 - data.find_last_not_of('=');
  using it_binary_t = transform_width<binary_from_base64<convert<std::string_view::const_iterator>>, 8, 6>;
  m_data.assign(it_binary_t(data.begin()), it_binary_t(data.end()));
  m_data.erase(m_data.end() - padding_len, m_data.end());
}

// Encode
std::string BinaryData::to_base64_string() const
{
  using it_base64_t = base64_from_binary<transform_width<data_type::const_iterator, 6, 8>>;
  std::string base64(it_base64_t(m_data.begin()), it_base64_t(m_data.end()));
  base64.append(required_padding(m_data.size() % 3), '=');
  return base64;
}

std::string BinaryData::to_hexadecimal_string() const
{
  std::string result;
  result.reserve(2 * m_data.size());
  for (auto b : m_data)
  {
    int val = /*std::to_integer<int>*/(b);
    result += nibble(val >> 4);
    result += nibble(val & 0xf);
  }
  return result;
}

void BinaryData::print_on(std::ostream& os) const
{
  os << '{' << to_hexadecimal_string() << '}';
}
