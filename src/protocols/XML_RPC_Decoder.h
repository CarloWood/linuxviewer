#pragma once

#include "evio/protocol/UTF8_SAX_Decoder.h"
#include "xmlrpc/ElementDecoder.h"
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
  xmlrpc::ElementDecoder* m_current_xml_rpc_element_decoder;
  std::stack<xmlrpc::ElementDecoder*> m_xml_rpc_response_stack;
  xmlrpc::ElementBase* m_current_element;

  void restore_current_xml_rpc_element_decoder()
  {
    m_current_xml_rpc_element_decoder = m_xml_rpc_response_stack.top();
    ASSERT(m_current_xml_rpc_element_decoder);
    m_xml_rpc_response_stack.pop();
  }

 protected:
  void start_document(size_t content_length, std::string version, std::string encoding) final;
  void end_document() final;
  void start_element(index_type element_id) final;
  void end_element(index_type element_id) final;
  void characters(std::string_view const& data) final;

 public:
  XML_RPC_Decoder(xmlrpc::ElementDecoder& xml_rpc_element_decoder) : m_current_xml_rpc_element_decoder(&xml_rpc_element_decoder) { }

  void start_struct()
  {
    m_xml_rpc_response_stack.push(m_current_xml_rpc_element_decoder);
    m_current_xml_rpc_element_decoder = m_current_xml_rpc_element_decoder->get_struct_decoder();
    ASSERT(m_current_xml_rpc_element_decoder);
  }

  void start_member(std::string_view const& name)
  {
    m_xml_rpc_response_stack.push(m_current_xml_rpc_element_decoder);
    m_current_xml_rpc_element_decoder = m_current_xml_rpc_element_decoder->get_member_decoder(name);
    ASSERT(m_current_xml_rpc_element_decoder);
  }

#ifdef CWDEBUG
  void got_member_type(xmlrpc::data_type type, char const* struct_name)
  {
    m_current_xml_rpc_element_decoder->got_member_type(type, struct_name);
  }
#endif

  void end_member()
  {
    restore_current_xml_rpc_element_decoder();
  }

  void end_struct()
  {
    restore_current_xml_rpc_element_decoder();
  }

  void start_array()
  {
    m_xml_rpc_response_stack.push(m_current_xml_rpc_element_decoder);
    m_current_xml_rpc_element_decoder = m_current_xml_rpc_element_decoder->get_array_decoder();
    ASSERT(m_current_xml_rpc_element_decoder);
  }

  void end_array()
  {
    restore_current_xml_rpc_element_decoder();
  }

  void got_characters(std::string_view const& data) const
  {
    m_current_xml_rpc_element_decoder->got_characters(data);
  }

  void got_data()
  {
    m_current_xml_rpc_element_decoder->got_data();
  }
};
