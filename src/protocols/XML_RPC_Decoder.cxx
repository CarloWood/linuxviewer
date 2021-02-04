#include "sys.h"
#include "XML_RPC_Decoder.h"
#include "utils/print_using.h"
#include "utils/c_escape.h"
#include <charconv>

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct xmlrpc("XMLRPC");
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace xmlrpc {

// See https://en.wikipedia.org/wiki/XML-RPC

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

class ElementBase2 : public ElementBase
{
 protected:
  using ElementBase::ElementBase;

  std::string name() const override
  {
    ASSERT(0 <= m_id && m_id < number_of_elements);
    return element_to_string(static_cast<elements>(m_id));
  }

  bool has_allowed_parent() override
  {
    return false;
  }

  void characters(XML_RPC_Decoder const* UNUSED_ARG(decoder), std::string_view const& data) override
  {
    THROW_ALERT("Element <[ELEMENT]> contains unexpected characters \"[DATA]\"",
        AIArgs("[ELEMENT]", name())("[DATA]", utils::print_using(data, utils::c_escape)));
  }

  void start_element(XML_RPC_Decoder* UNUSED_ARG(decoder)) override { }
  void end_element(XML_RPC_Decoder* UNUSED_ARG(decoder)) override { }
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
    // <params> can only occur in <methodResponse>.
    return m_parent && m_parent->id() == element_methodResponse;
  }
};

template<>
class Element<element_param> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // <param> can only occur in <params>.
    return m_parent && m_parent->id() == element_params;
  }
};

template<>
class Element<element_member> : public ElementBase2
{
  bool m_have_name;
#ifdef CWDEBUG
  data_type m_data_type;
#endif

  bool has_allowed_parent() override
  {
    // <member> can only occur in <struct>.
    return m_parent && m_parent->id() == element_struct;
  }

  void end_element(XML_RPC_Decoder* decoder) override
  {
    if (!m_have_name)
      THROW_ALERT("<member> without <name>");
#ifdef CWDEBUG
    char const* struct_name;
    if (m_data_type == data_type_struct)
      struct_name = decoder->get_struct_name();
#endif
    decoder->end_member();
    Debug(decoder->got_member_type(m_data_type, struct_name));
  }

 public:
  Element(index_type id, ElementBase* parent) : ElementBase2(id, parent), m_have_name(false) { }

  void have_name()
  {
    m_have_name = true;
  }

#ifdef CWDEBUG
  void got_member_type(data_type type)
  {
    m_data_type = type;
  }
#endif
};

template<>
class Element<element_data> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // <data> can only occur in <array>.
    return m_parent && m_parent->id() == element_array;
  }

  void start_element(XML_RPC_Decoder* decoder) override
  {
    decoder->got_data();
  }
};

template<>
class Element<element_value> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // <value> can only occur in <param>, <member> or <data>.
    return m_parent &&
      (m_parent->id() == element_param ||
       m_parent->id() == element_member ||
       m_parent->id() == element_data);
  }

 public:
  data_type m_data_type;

#ifdef CWDEBUG
  void end_element(XML_RPC_Decoder* decoder) override
  {
    if (m_parent->id() == element_member)
    {
      Element<element_member>* parent = static_cast<Element<element_member>*>(m_parent);
      parent->got_member_type(m_data_type);
    }
  }
#endif

#if 0
 public:
  template<typename T>
  void transfer_value(T&& data)
  {
    switch (m_parent->id())
    {
      case element_param:
      {
        Element<element_param>* parent = static_cast<Element<element_param>*>(m_parent);
        // Not implemented yet. What to do when transfer_value is called on <param><value>?
        ASSERT(false);
        break;
      }
      case element_member:
      {
        Element<element_member>* parent = static_cast<Element<element_member>*>(m_parent);
        //parent->transfer_value(std::forward<T>(data));
        // Not implemented yet. What to do when transfer_value is called on <member><value>?
        ASSERT(false);
        break;
      }
      case element_data:
      {
        Element<element_data>* parent = static_cast<Element<element_data>*>(m_parent);
        // Not implemented yet. What to do when transfer_value is called on <data><value>?
        ASSERT(false);
        break;
      }
      default:
        // Impossible due to allowed_parent.
        ASSERT(false);
    }
  }
#endif
};

template<>
class Element<element_struct> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // <struct> can only occur in <value>.
    return m_parent && m_parent->id() == element_value;
  }

  void start_element(XML_RPC_Decoder* decoder) override
  {
    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->m_data_type = data_type_struct;
    decoder->start_struct();
  }

  void end_element(XML_RPC_Decoder* decoder) override
  {
    decoder->end_struct();
  }
};

template<>
class Element<element_name> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // <name> can only occur in <member>.
    return m_parent && m_parent->id() == element_member;
  }

  std::string m_name;
  void characters(XML_RPC_Decoder const* decoder, std::string_view const& data) override
  {
    if (data.size() > 256)
      THROW_ALERT("Refusing to allocate a <name> of more than 256 characters (\"[DATA]\")",
          AIArgs("[DATA]", utils::print_using(data, utils::c_escape)));
    m_name = data;
  }

  void end_element(XML_RPC_Decoder* decoder) override
  {
    if (m_name.empty())
      THROW_ALERT("Empty element <name>");
    decoder->start_member(m_name);
    Element<element_member>* parent = static_cast<Element<element_member>*>(m_parent);
    parent->have_name();
  }
};

class ElementVariable : public ElementBase2
{
  bool has_allowed_parent() override
  {
    // Variables (see below) can only occur in <value>.
    return m_parent && m_parent->id() == element_value;
  }

  void end_element(XML_RPC_Decoder* decoder) override
  {
    if (!m_received_characters)
      characters(decoder, {});
  }

  void characters(XML_RPC_Decoder const* decoder, std::string_view const& data) override
  {
    m_received_characters = true;
    decoder->got_characters(data);
  }

 protected:
  bool m_received_characters;

 public:
  ElementVariable(index_type id, ElementBase* parent) : ElementBase2(id, parent), m_received_characters(false) { }
};

template<>
class Element<element_array> : public ElementVariable
{
  using ElementVariable::ElementVariable;

  void start_element(XML_RPC_Decoder* decoder) override
  {
    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->m_data_type = data_type_array;
    decoder->start_array();
  }

  void characters(XML_RPC_Decoder const* decoder, std::string_view const& data) override
  {
    // <array> is not allowed to contain characters.
    THROW_ALERT("Element <array> contains unexpected characters \"[DATA]\"",
        AIArgs("[DATA]", utils::print_using(data, utils::c_escape)));
  }

  void end_element(XML_RPC_Decoder* decoder) override
  {
    // <array> is not allowed to contain characters.
    decoder->end_array();
  }
};

template<>
class Element<element_base64> : public ElementVariable
{
  using ElementVariable::ElementVariable;

  void start_element(XML_RPC_Decoder* decoder) override
  {
    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->m_data_type = data_type_base64;
  }
};

template<>
class Element<element_boolean> : public ElementVariable
{
  using ElementVariable::ElementVariable;

  void start_element(XML_RPC_Decoder* decoder) override
  {
    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->m_data_type = data_type_boolean;
  }
};

template<>
class Element<element_dateTime_iso8601> : public ElementVariable
{
  using ElementVariable::ElementVariable;

  void start_element(XML_RPC_Decoder* decoder) override
  {
    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->m_data_type = data_type_datetime;
  }
};

template<>
class Element<element_double> : public ElementVariable
{
  using ElementVariable::ElementVariable;

  void start_element(XML_RPC_Decoder* decoder) override
  {
    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->m_data_type = data_type_double;
  }
};

// A signed integer of 32 bytes.
class ElementInt : public ElementVariable
{
  using ElementVariable::ElementVariable;

  void start_element(XML_RPC_Decoder* decoder) override
  {
    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->m_data_type = data_type_int;
  }
};

template<>
class Element<element_int> : public ElementInt
{
  using ElementInt::ElementInt;
};

template<>
class Element<element_i4> : public ElementInt
{
  using ElementInt::ElementInt;
};

template<>
class Element<element_string> : public ElementVariable
{
  using ElementVariable::ElementVariable;

  void start_element(XML_RPC_Decoder* decoder) override
  {
    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->m_data_type = data_type_string;
  }
};

#define SIZEOF_ELEMENT_COMMA(el) sizeof(Element<element_##el>),

constexpr size_t largest_size = std::max({FOREACH_ELEMENT(SIZEOF_ELEMENT_COMMA)});

utils::NodeMemoryPool pool(128, largest_size);

#define XMLRPC_CASE_CREATE(el) \
  case element_##el: \
    return new(pool) Element<element_##el>(element_##el, parent);

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

} // namespace xmlrpc

void XML_RPC_Decoder::start_document(size_t content_length, std::string version, std::string encoding)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::start_document(" << content_length << ", " << version << ", " << encoding << ")");

  for (size_t i = 0; i < xmlrpc::number_of_elements; ++i)
  {
    char const* name = element_to_string(static_cast<xmlrpc::elements>(i));
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
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::start_element(" << element_id << " [" << name_of(element_id) << "])");

  xmlrpc::ElementBase* parent = m_current_element;
  m_current_element = create_element(element_id, parent);
  if (!m_current_element->has_allowed_parent())
    THROW_ALERT("Element <[PARENT]> is not expected to have child element <[CHILD]>",
        AIArgs("[PARENT]", parent->name())("[CHILD]", m_current_element->name()));
  m_current_element->start_element(this);
}

void XML_RPC_Decoder::end_element(index_type element_id)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::end_element(" << element_id << " [" << name_of(element_id) << "])");
  m_current_element->end_element(this);
  m_current_element = destroy_element(m_current_element);
}

void XML_RPC_Decoder::characters(std::string_view const& data)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::characters(\"" << libcwd::buf2str(data.data(), data.size()) << "\")");
  m_current_element->characters(this, data);
}
