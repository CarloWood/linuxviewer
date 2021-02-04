#pragma once

#include "evio/protocol/UTF8_SAX_Decoder.h"
#include "XML_RPC_Response.h"
#include <stack>

class XML_RPC_Decoder;

namespace xmlrpc {

class ElementBase
{
 public:
  using index_type = evio::protocol::UTF8_SAX_Decoder::index_type;

 protected:
  index_type m_id;
  ElementBase* m_parent;

 public:
  ElementBase(index_type id, ElementBase* parent) : m_id(id), m_parent(parent) { }
  virtual ~ElementBase() { }
  // Objects derived from ElementBase must be allocated with:
  // utils::NodeMemoryPool pool(128, sizeof(LargestDerivedClass));
  // DerivedClass* foo = new(pool) DerivedClass(...constructor args...);        // Allocate memory from memory pool and construct object.
  // delete foo;
  void operator delete(void* ptr) { utils::NodeMemoryPool::static_free(ptr); }

  index_type id() { return m_id; }
  ElementBase* parent() { return m_parent; }

  virtual std::string name() const = 0;
  virtual bool has_allowed_parent() = 0;
  virtual void start_element(XML_RPC_Decoder* decoder) = 0;
  virtual void characters(XML_RPC_Decoder const* decoder, std::string_view const& data) = 0;
  virtual void end_element(XML_RPC_Decoder* decoder) = 0;
};

} // namespace xmlrpc

class XML_RPC_Decoder : public evio::protocol::UTF8_SAX_Decoder
{
 private:
  XML_RPC_Response* m_current_xml_rpc_data;
  std::stack<XML_RPC_Response*> m_xml_rpc_response_stack;
  xmlrpc::ElementBase* m_current_element;

  void restore_current_xml_rpc_data()
  {
    m_current_xml_rpc_data = m_xml_rpc_response_stack.top();
    ASSERT(m_current_xml_rpc_data);
    m_xml_rpc_response_stack.pop();
  }

 protected:
  void start_document(size_t content_length, std::string version, std::string encoding) final;
  void end_document() final;
  void start_element(index_type element_id) final;
  void end_element(index_type element_id) final;
  void characters(std::string_view const& data) final;

 public:
  XML_RPC_Decoder(XML_RPC_Response& xml_rpc_data) : m_current_xml_rpc_data(&xml_rpc_data) { }

  void start_struct()
  {
    m_xml_rpc_response_stack.push(m_current_xml_rpc_data);
    m_current_xml_rpc_data = m_current_xml_rpc_data->get_struct();
    ASSERT(m_current_xml_rpc_data);
  }

  void start_member(std::string_view const& name)
  {
    m_xml_rpc_response_stack.push(m_current_xml_rpc_data);
    m_current_xml_rpc_data = m_current_xml_rpc_data->get_member(name);
    ASSERT(m_current_xml_rpc_data);
  }

#ifdef CWDEBUG
  void got_member_type(xmlrpc::data_type type, char const* struct_name)
  {
    m_current_xml_rpc_data->got_member_type(type, struct_name);
  }

  char const* get_struct_name() const
  {
    return m_current_xml_rpc_data->get_struct_name();
  }
#endif

  void end_member()
  {
    restore_current_xml_rpc_data();
  }

  void end_struct()
  {
    restore_current_xml_rpc_data();
  }

  void start_array()
  {
    m_xml_rpc_response_stack.push(m_current_xml_rpc_data);
    m_current_xml_rpc_data = m_current_xml_rpc_data->get_array();
    ASSERT(m_current_xml_rpc_data);
  }

  void end_array()
  {
    restore_current_xml_rpc_data();
  }

  void got_characters(std::string_view const& data) const
  {
    m_current_xml_rpc_data->got_characters(data);
  }

  void got_data()
  {
    m_current_xml_rpc_data->got_data();
  }
};
