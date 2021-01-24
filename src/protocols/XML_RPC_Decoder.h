#pragma once

#include "evio/protocol/UTF8_SAX_Decoder.h"
#include <stack>

class XML_RPC_Response;

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
  virtual void characters(std::string_view const& data) = 0;
  virtual void end_element() = 0;
};

} // namespace xmlrpc

class XML_RPC_Decoder : public evio::protocol::UTF8_SAX_Decoder
{
 private:
  XML_RPC_Response& m_response;                     // The XML_RPC_ ...
  std::stack<XML_RPC_Response*> m_current_response; // Pointer to XML_RPC_Data<> objects.
  xmlrpc::ElementBase* m_current_element;

 protected:
  void start_document(size_t content_length, std::string version, std::string encoding) final;
  void end_document() final;
  void start_element(index_type element_id) final;
  void end_element(index_type element_id) final;
  void characters(std::string_view const& data) final;

 public:
  XML_RPC_Decoder(XML_RPC_Response& response) : m_response(response) { }
};
