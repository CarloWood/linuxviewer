#include "sys.h"
#include "XML_RPC_Decoder.h"

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct xmlrpc("XMLRPC");
NAMESPACE_DEBUG_CHANNELS_END
#endif

enum elements {
  element_methodResponse,
  element_params,
  element_param,
  element_value,
  element_struct,
  element_member,
  element_name,
  element_array,
  element_data,
  element_base64,
  element_boolean,
  element_dateTime_iso8601,
  element_double,
  element_int,
  element_i4,
  element_string,
};

size_t constexpr number_of_elements = element_string + 1;

#define XMLRPC_CASE_RETURN(x) case x: return & #x [8];

char const* element_to_string(elements element)
{
  switch (element)
  {
    XMLRPC_CASE_RETURN(element_methodResponse);
    XMLRPC_CASE_RETURN(element_params);
    XMLRPC_CASE_RETURN(element_param);
    XMLRPC_CASE_RETURN(element_value);
    XMLRPC_CASE_RETURN(element_struct);
    XMLRPC_CASE_RETURN(element_member);
    XMLRPC_CASE_RETURN(element_name);
    XMLRPC_CASE_RETURN(element_array);
    XMLRPC_CASE_RETURN(element_data);
    XMLRPC_CASE_RETURN(element_base64);
    XMLRPC_CASE_RETURN(element_boolean);
    case element_dateTime_iso8601: return "dateTime.iso8601";
    XMLRPC_CASE_RETURN(element_double);
    XMLRPC_CASE_RETURN(element_int);
    XMLRPC_CASE_RETURN(element_i4);
    XMLRPC_CASE_RETURN(element_string);
  }
}

void XML_RPC_Decoder::start_document(size_t content_length, std::string version, std::string encoding)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::start_document(" << content_length << ", " << version << ", " << encoding << ")");

  for (size_t i = 0; i < number_of_elements; ++i)
  {
    char const* name = element_to_string(static_cast<elements>(i));
    add(i, name);
  }
}

void XML_RPC_Decoder::end_document()
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::end_document()");
}

void XML_RPC_Decoder::start_element(index_type element_id)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::start_element(" << element_id << " [" << element(element_id) << "])");

  evio::protocol::xml::Element& element = this->element(element_id);
  std::string name{element.name()};
  switch (element_id)
  {
    case element_methodResponse:
      ASSERT(name == "methodResponse");
      break;
    case element_params:
      ASSERT(name == "params");
      break;
    case element_param:
      ASSERT(name == "param");
      break;
    case element_value:
      ASSERT(name == "value");
      break;
    case element_struct:
      ASSERT(name == "struct");
      break;
    case element_member:
      ASSERT(name == "member");
      break;
    case element_name:
      ASSERT(name == "name");
      break;
    case element_array:
      ASSERT(name == "array");
      break;
    case element_data:
      ASSERT(name == "data");
      break;
    case element_base64:
      ASSERT(name == "base64");
      break;
    case element_boolean:
      ASSERT(name == "boolean");
      break;
    case element_dateTime_iso8601:
      ASSERT(name == "dateTime.iso8601");
      break;
    case element_double:
      ASSERT(name == "double");
      break;
    case element_int:
      ASSERT(name == "int");
      break;
    case element_i4:
      ASSERT(name == "i4");
      break;
    case element_string:
      ASSERT(name == "string");
      break;
    default:
      DoutFatal(dc::core, "Unknown element \"" << name << "\"");
      break;
  }
}

void XML_RPC_Decoder::end_element(index_type element_id)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::end_element({" << element(element_id).name() << "})");
}

void XML_RPC_Decoder::characters(std::string_view const& data)
{
  DoutEntering(dc::xmlrpc, "XML_RPC_Decoder::characters(\"" << libcwd::buf2str(data.data(), data.size()) << "\")");
}
