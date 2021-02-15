#include "sys.h"
#include "Gender.h"
#include "utils/AIAlert.h"
#include "utils/print_using.h"
#include "utils/c_escape.h"
#ifdef CWDEBUG
#include <iosfwd>
#endif

void Gender::assign_from_xmlrpc_string(std::string_view const& data)
{
  if (data == "female")
    set_gender(gender_female);
  else if (data == "male")
    set_gender(gender_male);
  else
    THROW_ALERT("Invalid Gender '[GENDER]'", AIArgs("[GENDER]", utils::print_using(data, utils::c_escape)));
}

#ifdef CWDEBUG
void Gender::print_on(std::ostream& os) const
{
  os << '{';
  switch (m_gender)
  {
    case gender_female:
      os << "gender_female";
      break;
    case gender_male:
      os << "gender_male";
      break;
  }
  os << '}';
}
#endif
