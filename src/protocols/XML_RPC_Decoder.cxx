#include "sys.h"
#include "XML_RPC_Decoder.h"
#include "XML_RPC_Struct.h"
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

  void characters(std::string_view const& data) override
  {
    // FIXME? Escape 'data' so that the output is always printable.
    THROW_ALERT("Element <[ELEMENT]> contains unexpected characters \"[DATA]\"",
        AIArgs("[ELEMENT]", name())("[DATA]", data));
  }

  void end_element() override { }
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
class Element<element_struct> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // <struct> can only occur in <value>.
    return m_parent && m_parent->id() == element_value;
  }

  XML_RPC_Struct* m_xml_rpc_struct;

 public:
  template<typename T>
  void transfer_key_value_pair(std::string&& name, T&& data)
  {
    m_xml_rpc_struct->transfer_key_value_pair(std::move(name), std::forward<T>(data));
  }
};

template<>
class Element<element_member> : public ElementBase2
{
  using ElementBase2::ElementBase2;
  bool has_allowed_parent() override
  {
    // <member> can only occur in <struct>.
    return m_parent && m_parent->id() == element_struct;
  }

  std::string m_name;

 public:
  // Called from Element<element_name>::end_element.
  void transfer_name(std::string&& name)
  {
    m_name = std::move(name);
  }

  template<typename T>
  void transfer_value(T&& data)
  {
    if (m_name.empty())
      THROW_ALERT("In element <member>, <name> is expected before <value>");
    Element<element_struct>* parent = static_cast<Element<element_struct>*>(m_parent);
    parent->transfer_key_value_pair(std::move(m_name), std::forward<T>(data));
  }
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
        parent->transfer_value(std::forward<T>(data));
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
  void characters(std::string_view const& data) override
  {
    if (data.size() > 256)
      THROW_ALERT("Refusing to allocate a <name> of more than 256 characters (\"[DATA]\")", AIArgs("[DATA]", data));
    m_name = data;
  }

  void end_element() override
  {
    if (m_name.empty())
      THROW_ALERT("Empty element <name>");
    Element<element_member>* parent = static_cast<Element<element_member>*>(m_parent);
    parent->transfer_name(std::move(m_name));
  }
};

class ElementVariable : public ElementBase2
{
  bool has_allowed_parent() override
  {
    // Variables (see below) can only occur in <value>.
    return m_parent && m_parent->id() == element_value;
  }

  void end_element() override
  {
    if (!m_received_characters)
      characters({});
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

  void end_element() override
  {
    // <array> is not allowed to contain characters.
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

  void characters(std::string_view const& data) override
  {
    m_received_characters = true;
    bool val{false};
    if (data == "true")
      val = true;
    else if (data != "false")
      // FIXME? Escape 'data' so that the output is always printable.
      THROW_ALERT("Invalid characters in element <bool> (\"[DATA]\")", AIArgs("[DATA]", data));
    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->transfer_value(val);
  }
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

// A signed integer of 32 bytes.
class ElementInt : public ElementVariable
{
  using ElementVariable::ElementVariable;

  void characters(std::string_view const& data) override
  {
    m_received_characters = true;
    int32_t val;
    std::from_chars_result result = std::from_chars(data.data(), data.data() + data.size(), val);
    // Throw if there wasn't any digit, or if the result doesn't fit in a int32_t.
    if (AI_UNLIKELY(result.ec != std::errc()))
      THROW_ALERTC(result.ec, "Element<element_i4>::characters: \"[DATA]\"", AIArgs("[DATA]", data));

    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->transfer_value(val);
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

  void characters(std::string_view const& data) override
  {
    m_received_characters = true;
    Element<element_value>* parent = static_cast<Element<element_value>*>(m_parent);
    parent->transfer_value(data);
  }
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
}

void XML_RPC_Decoder::end_element(index_type element_id)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::end_element(" << element_id << " [" << name_of(element_id) << "])");
  m_current_element->end_element();
  m_current_element = destroy_element(m_current_element);
}

void XML_RPC_Decoder::characters(std::string_view const& data)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::characters(\"" << libcwd::buf2str(data.data(), data.size()) << "\")");
  m_current_element->characters(data);
}
