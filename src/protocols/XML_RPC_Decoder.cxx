#include "sys.h"
#include "XML_RPC_Decoder.h"

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct xmlrpc("XMLRPC");
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace {

#define FOREACH_ELEMENT(X) \
  X(methodResponse) \
  X(params) \
  X(param) \
  X(value) \
  X(struct) \
  X(member) \
  X(name) \
  X(array) \
  X(data) \
  X(base64) \
  X(boolean) \
  X(dateTime_iso8601) \
  X(double) \
  X(int) \
  X(i4) \
  X(string)

#define ENUMERATOR_NAME_COMMA(el) element_##el,

// Definition of all elements as enumerators.
enum elements {
  FOREACH_ELEMENT(ENUMERATOR_NAME_COMMA)
};

size_t constexpr number_of_elements = element_string + 1;

#define XMLRPC_CASE_RETURN(el) case element_##el: return element_##el == element_dateTime_iso8601 ? "dateTime.iso8601" : #el;

char const* element_to_string(elements element)
{
  switch (element)
  {
    FOREACH_ELEMENT(XMLRPC_CASE_RETURN)
  }
}

template<elements enum_type>
class Element;

using ElementBase = evio::protocol::xml::ElementBase;

class ElementBase2 : public ElementBase
{
 protected:
  using evio::protocol::xml::ElementBase::ElementBase;

  std::string name() const override
  {
    ASSERT(0 <= m_id && m_id < number_of_elements);
    return element_to_string(static_cast<elements>(m_id));
  }

  bool has_allowed_parent() override
  {
    return false;
  }
};

class UnknownElement : public ElementBase2
{
  using ElementBase2::ElementBase2;

  std::string name() const override
  {
    return "Unknown";
  }
};

template<>
class Element<element_methodResponse> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // <methodResponse> can only be the first tag.
    return m_parent == nullptr;
  }
};

template<>
class Element<element_params> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // Only <methodResponse> can contain <params>.
    return m_parent && m_parent->id() == element_methodResponse;
  }
};

template<>
class Element<element_param> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // Only <params> can contain <param>.
    return m_parent && m_parent->id() == element_params;
  }
};

template<>
class Element<element_value> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // Only <param> and <member> can contain <value>.
    return m_parent &&
      (m_parent->id() == element_param ||
       m_parent->id() == element_member ||
       m_parent->id() == element_data);
  }
};

template<>
class Element<element_struct> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // Only <value> can contain <struct>.
    return m_parent && m_parent->id() == element_value;
  }
};

template<>
class Element<element_member> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // Only <struct> can contain <member>.
    return m_parent && m_parent->id() == element_struct;
  }
};

template<>
class Element<element_name> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // Only <member> can contain <name>.
    return m_parent && m_parent->id() == element_member;
  }
};

class ElementVariable : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // Only <value> can contain variables (see below).
    return m_parent && m_parent->id() == element_value;
  }
};

template<>
class Element<element_array> : public ElementVariable
{
  using ElementVariable::ElementVariable;
};

template<>
class Element<element_data> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // Only <array> can contain <data>.
    return m_parent && m_parent->id() == element_array;
  }
};

template<>
class Element<element_base64> : public ElementVariable
{
  using ElementVariable::ElementVariable;
};

template<>
class Element<element_boolean> : public ElementVariable
{
  using ElementVariable::ElementVariable;
};

template<>
class Element<element_dateTime_iso8601> : public ElementVariable
{
  using ElementVariable::ElementVariable;
};

template<>
class Element<element_double> : public ElementVariable
{
  using ElementVariable::ElementVariable;
};

template<>
class Element<element_int> : public ElementVariable
{
  using ElementVariable::ElementVariable;
};

template<>
class Element<element_i4> : public ElementVariable
{
  using ElementVariable::ElementVariable;
};

template<>
class Element<element_string> : public ElementVariable
{
  using ElementVariable::ElementVariable;
};

#define SIZEOF_ELEMENT_COMMA(el) sizeof(Element<element_##el>),

constexpr size_t largest_size = std::max({FOREACH_ELEMENT(SIZEOF_ELEMENT_COMMA)});

utils::NodeMemoryPool pool(128, largest_size);

#define XMLRPC_CASE_CREATE(el) case element_##el: return new(pool) Element<element_##el>(element_##el, parent);

ElementBase* create_element(XML_RPC_Decoder::index_type element, ElementBase* parent)
{
  switch (element)
  {
    FOREACH_ELEMENT(XMLRPC_CASE_CREATE)
    default:
      return new UnknownElement{element, parent};
  }
}

ElementBase* destroy_element(ElementBase* current_element)
{
  ElementBase* parent = current_element->parent();
  delete current_element;
  return parent;
}

} // namespace

void XML_RPC_Decoder::start_document(size_t content_length, std::string version, std::string encoding)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::start_document(" << content_length << ", " << version << ", " << encoding << ")");

  for (size_t i = 0; i < number_of_elements; ++i)
  {
    char const* name = element_to_string(static_cast<elements>(i));
    add(i, name);
  }

  m_current_element = nullptr;
}

void XML_RPC_Decoder::end_document()
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::end_document()");
}

void XML_RPC_Decoder::start_element(index_type element_id)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::start_element(" << element_id << " [" << element_type(element_id) << "])");

  ElementBase* parent = m_current_element;
  m_current_element = create_element(element_id, parent);
  Dout(dc::notice, m_current_element->tree());
  if (!m_current_element->has_allowed_parent())
    THROW_ALERT("Unexpected XML RPC Response: element <[PARENT]> is not expected to have child element <[CHILD]>",
        AIArgs("[PARENT]", parent->name())("[CHILD]", m_current_element->name()));
}

void XML_RPC_Decoder::end_element(index_type element_id)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::end_element(" << element_id << " [" << element_type(element_id) << "])");
  m_current_element = destroy_element(m_current_element);
}

void XML_RPC_Decoder::characters(std::string_view const& data)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::characters(\"" << libcwd::buf2str(data.data(), data.size()) << "\")");
}
