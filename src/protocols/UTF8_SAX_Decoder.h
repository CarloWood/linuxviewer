#pragma once

#include "evio/protocol/Decoder.h"
#include <unordered_map>
#include <string_view>
#include <iosfwd>

namespace xml {

class Element
{
 private:
  int m_id;
  std::string m_name;

 public:
  Element(int id, std::string name) : m_id(id), m_name(std::move(name)) { }

  int id() const { return m_id; }
  std::string const& name() const { return m_name; }

  void print_on(std::ostream& os) const;

  friend std::ostream& operator<<(std::ostream& os, Element const& element)
  {
    element.print_on(os);
    return os;
  }
};

} // namespace xml

// This decoder deserializes XML.
//
// It only supports UTF-8.
//
class UTF8_SAX_Decoder : public evio::protocol::Decoder
{
 public:
  using element_id_type = int;          // Index into m_unique_elements.

 private:
  bool m_document_begin;
  std::vector<xml::Element> m_unique_elements;
  std::unordered_map<std::string_view, xml::Element> m_elements;

 public:
  UTF8_SAX_Decoder() : m_document_begin(true) { }

 private:
  element_id_type get_element_id(std::string_view name);

 protected:
  size_t end_of_msg_finder(char const* new_data, size_t rlen, evio::EndOfMsgFinderResult& result) final;
  void decode(int& allow_deletion_count, evio::MsgBlock&& msg) override;

 protected:
  virtual void start_document(size_t content_length, std::string version, std::string encoding) { DoutEntering(dc::notice, "UTF8_SAX_Decoder::start_document(" << content_length << ")"); }
  virtual void end_document() { DoutEntering(dc::notice, "UTF8_SAX_Decoder::end_document()"); }
  virtual void start_element(element_id_type element_id) { DoutEntering(dc::notice, "UTF8_SAX_Decoder::start_element({" << m_unique_elements[element_id] << "})"); }
  virtual void end_element(element_id_type element_id) { DoutEntering(dc::notice, "UTF8_SAX_Decoder::end_element({" << m_unique_elements[element_id] << "})"); }
  virtual void characters(std::string_view const& data) { DoutEntering(dc::notice, "UTF8_SAX_Decoder::characters(\"" << libcwd::buf2str(data.data(), data.size()) << "\")"); }
};
