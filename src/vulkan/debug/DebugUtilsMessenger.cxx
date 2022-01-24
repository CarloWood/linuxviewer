#include "sys.h"

#ifdef CWDEBUG
#include "Defaults.h"
#include "DebugUtilsMessenger.h"
#include "debug.h"
#include "debug/vulkan_print_on.h"
#include <magic_enum.hpp>
#include <string_view>
#include <vector>
#include <regex>
#include <sstream>
#include <iomanip>
#include <map>

namespace vulkan {

void DebugUtilsMessenger::prepare(vk::Instance vh_instance, vk::DebugUtilsMessengerCreateInfoEXT const& debug_create_info)
{
  // Keep a copy, because we need that in the destructor.
  m_vh_instance = vh_instance;
  m_debug_utils_messenger = m_vh_instance.createDebugUtilsMessengerEXTUnique(debug_create_info);
}

namespace {

struct ValidationError;

struct Object
{
  int n;
  VkDebugUtilsObjectNameInfoEXT info;
};

struct Token
{
  bool m_is_object;
  std::string m_text;
  uint64_t m_id;

  Token(std::string_view const& text) : m_is_object(false), m_text(text) { }
  Token(std::string_view::const_iterator b, std::string_view::const_iterator e) : m_is_object(false), m_text(b, e) { }
  Token(uint64_t id, std::string_view const& object_str) : m_is_object(true), m_id(id), m_text(object_str) { }

  std::string to_string(ValidationError& validation_error) const;       // The argument is a reference to a stack variable.
};

struct ValidationError
{
  std::string function;
  std::map<uint64_t, Object> objects;
  std::vector<Token> error;
  std::string spec;
  std::string url;
  std::vector<std::string> unknown_object_names;

  ValidationError(VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData);

  void parse(std::string_view input);

 private:
  static std::string::size_type find_message_ID(std::string_view const& input);
};

std::string Token::to_string(ValidationError& validation_error) const
{
  if (m_is_object)
  {
    std::ostringstream oss;
    auto iter = validation_error.objects.find(m_id);
    if (iter == validation_error.objects.end())
    {
      // Ugh, this validation error uses an object that is not listed in VkDebugUtilsMessengerCallbackDataEXT::pObjects.
      // Now we have to decode it as well. The expected string format is (stored in m_text for this purpose) is:
      // VkDescriptorSetLayout 0xb3c7bc000000007f[m_imgui.m_descriptor_set.m_layout [0x557fd7044b90]]
      // ^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^
      // |                     |                  |                                                  |
      // objectType            objectHandle       pObjectName                                        |
      // (C identifier)        (hex uint64_t)                                                        |
      // |                                                                                           |
      // m_text.data()                                                                         m_text.end()

      auto objectHandle_pos = m_text.find_first_of(' ') + 1;
      auto pObjectName_pos = m_text.find_first_of('[') + 1;

      std::string_view object_type_sv(m_text.data(), objectHandle_pos - 1);
      std::string object_id_str(m_text.data() + objectHandle_pos, pObjectName_pos - objectHandle_pos - 1);
      std::string object_name_str(m_text.data() + pObjectName_pos, m_text.length() - pObjectName_pos - 1);

      std::istringstream iss(object_id_str);
      uint64_t object_id;
      iss >> std::hex >> object_id;

      validation_error.unknown_object_names.push_back(object_name_str);

      VkObjectType object_type = VK_OBJECT_TYPE_UNKNOWN;
      std::string object_type_name = "e" + std::string(object_type_sv.substr(2));
      auto object_type_opt = magic_enum::enum_cast<vk::ObjectType>(object_type_name);
      if (object_type_opt.has_value())
        object_type = static_cast<VkObjectType>(object_type_opt.value());

      Object new_object = { static_cast<int>(validation_error.objects.size() + 1),
        {
          .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
          .pNext = nullptr,
          .objectType = object_type,
          .objectHandle = object_id,
          .pObjectName = validation_error.unknown_object_names.back().c_str()
        }
      };
      validation_error.objects[object_id] = new_object;
    }
    Object const& object = validation_error.objects.at(m_id);
    std::string_view object_name(object.info.pObjectName);
    auto window_ptr_pos = object_name.find(" [0x");
    if (window_ptr_pos != std::string::npos)
      object_name = object_name.substr(0, window_ptr_pos);
    oss << object_name << " (\e[30;43m" << object.n << "\e[39;49m)";
    return oss.str();
  }
  return m_text;
}

ValidationError::ValidationError(VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData)
{
  unknown_object_names.reserve(8);
  for (int i = 0; i < pCallbackData->objectCount; ++i)
  {
    Object object{ i + 1, pCallbackData->pObjects[i] };
    objects[object.info.objectHandle] = object;
  }
}

std::string::size_type ValidationError::find_message_ID(std::string_view const& input)
{
  // input: ... | MessageID = 0x7c54445e | vkCmdDrawIndexed(): ...
  // return:                               ^

  std::regex messageID_regex(R"(\| MessageID = 0x[0-9a-f]+ \| *)");
  std::match_results<std::string_view::const_iterator> mr;
  if (!std::regex_search(input.cbegin(), input.cend(), mr, messageID_regex))
  {
    Dout(dc::warning, "No MessageID pattern in: " << input);
    return 0;
  }
  return mr.position() + mr.length();
}

void ValidationError::parse(std::string_view message)
{
//  DoutEntering(dc::notice, "ValidationError::parse(\"" << message << "\")");

  // Find the start of the real message by skipping over everything up till and including the |MessageID| bit.
  auto skip_pos = find_message_ID(message);
  message.remove_prefix(skip_pos);

  // Get the function name and skip over that as well.
  auto space_pos = message.find_first_of(' ');
  auto colon_pos = message.find_first_of(':');
  if (colon_pos != std::string::npos && colon_pos < space_pos)
  {
    function = message.substr(0, colon_pos);
    auto start_pos = message.find_first_not_of(' ', colon_pos + 1);
    message.remove_prefix(start_pos);
  }

  // Find the url at the end, if any, and remove that from message.
  auto url_pos = message.rfind("(https://");
  if (url_pos != std::string::npos)
  {
    auto tail_sv = message.substr(url_pos);
    message.remove_suffix(tail_sv.length());
    if (tail_sv.ends_with(')'))
      tail_sv.remove_suffix(1);
    tail_sv.remove_prefix(1);
    url = tail_sv;
  }

  // Remove trailing while space (that sat before the '(https://').
  while (message.ends_with(' '))
    message.remove_suffix(1);

  // Find the text from spec, if any, and remove that from message as well.
  auto spec_pos = message.rfind("The Vulkan spec states:");
  if (spec_pos != std::string::npos)
  {
    auto spec_sv = message.substr(spec_pos + 23);
    message.remove_suffix(message.length() - spec_pos);
    while (message.ends_with(' '))
      message.remove_suffix(1);
    while (spec_sv.starts_with(' '))
      spec_sv.remove_prefix(1);
    spec = spec_sv;
    if (!spec_sv.ends_with('.'))
      spec += '.';
  }

  // message now exists of text with zero or more object references of the form
  // "VkType 0xhandle123[debug name with possibly nested [] once or twice[]]".
  // Since regular expressions are well with finding balanced brackets, we search
  // for all possible object starts (the "VkType 0xhandle123[") and find the
  // matching closing bracket manually.

  // Find all (possible) object starts.
  using Ptr = std::string_view::const_iterator;
  std::vector<Ptr> object_starts;
  std::regex object_start_regex(R"([A-Za-z0-9_]+ 0x[a-f0-9]{8,}\[)");
  std::transform(
      std::regex_iterator<Ptr>(message.cbegin(), message.cend(), object_start_regex),
      std::regex_iterator<Ptr>(),
      std::back_inserter(object_starts),
      [&message](auto& m) { return message.cbegin() + m.position(); }
  );

  // Since message is a string_view it is easiest to work with string_view::const_iterators
  // for all bookkeeping (where a const_iterator is just a `char const*`).
  //
  //     message.begin()                              (when found:) object_end     message.end()
  //          |                                                         |                 |
  //          v                                                         v                 v
  // message: ...<START_0>... <START_i-1>...]... ...<START_i>...[...]...]...<START_i+1>...
  //                          ^              ^      ^             ^         ^
  //                          |              |      |             |         |
  //                    object_starts[i-1]   |object_starts[i]    |   object_starts[i+1]
  //                                         |      ^             |         |
  //                                     text_start |          object_end <-|-- (while searching for the closing ']'.
  //                                                |                       |    here open_brackets == 1)
  //                                             text_end                   |
  //                                             (only permanent      object_end_limit
  //                                              once object_end
  //                                              was found).

  int const i_end = object_starts.size();
  Ptr text_start = message.begin();             // Per definition.
  Ptr text_end = message.end();                 // If there is no object (i_end == 0).
  Ptr object_end;
  for (int i = 0;; ++i)
  {
    // Stop searching at the end of the message or the next object start (whether or not that has a closing bracket).
    Ptr object_end_limit = (i + 1 < i_end) ? object_starts[i + 1] : message.end();
    // If there are more objects, update text_end to the correct value:
    while (i < i_end)
    {
      // Since there is another object string, text_end should end where that object begins.
      text_end = object_starts[i];
      // This is only preliminary: this is only the next object start if we can find a matching ']' before the next object start.
      // Initialize and then increment object_end until we found a matching ']'.
      object_end = text_end;
      int open_brackets = 0;
      while (object_end != object_end_limit && (*object_end != ']' || open_brackets > 1))
      {
        char c = *object_end++;
        if (c == '[')
          ++open_brackets;
        else if (c == ']')
          --open_brackets;
      }
      if (object_end != object_end_limit)
      {
        // Move object_end one past the matching closing bracket.
        ++object_end;
        break;
      }
      // If we could not find a closing bracket, then increment i and try with the next object string.
      ++i;
      text_end = message.end();
    }
    // Store the found text token, if any.
    if (text_start != text_end)
      error.emplace_back(text_start, text_end);

    if (text_end == message.end())
      break;

    Ptr object_start = text_end;

    std::string_view object_sv(object_start, object_end);
    auto bracket_pos = object_sv.find_first_of('[');
    auto handle_pos = object_sv.find_last_of('x', bracket_pos) - 1;
    std::string object_id_str(object_sv.substr(handle_pos, bracket_pos - handle_pos));
    std::istringstream iss(object_id_str);
    uint64_t object_id;
    iss >> std::hex >> object_id;
    error.emplace_back(object_id, object_sv);

    text_start = object_end;
    text_end = message.end();
  }
}

} // namespace

// Default callback function for debug output from vulkan layers.
//static
VkBool32 DebugUtilsMessenger::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* UNUSED_ARG(user_data))
{
  char const* color_end = "";
  libcwd::channel_ct* dcp;
  unsigned short indent;
  bool print_finish = true;
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    // Ignore this "error". See https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/1340
    if (pCallbackData->messageIdNumber == 0x7cd0911d)
      return VK_FALSE;

    // Set marker and indentation to zero while printing vulkan errors.
    indent = libcwd::libcw_do.get_indent();
    libcwd::libcw_do.push_marker();
    libcwd::libcw_do.marker().assign("\e[34;40m\u2620\u2620\u2620\e[39;49m ");

    dcp = &DEBUGCHANNELS::dc::vkerror;
    if (strncmp(pCallbackData->pMessage, "Validation Error:", 17) == 0)
    {
      ValidationError error(pCallbackData);
      error.parse(pCallbackData->pMessage);
      Dout(dc::vkerror|dc::warning|continued_cf, "\e[31mValidation Error: in function " << error.function << ":\n\e[39;49m\t");
      for (Token const& token : error.error)
        Dout(dc::continued, token.to_string(error));
      Dout(dc::continued, "\n");
      if (!error.url.empty())
        Dout(dc::continued, "\n" << error.url << " states:\n\n");
      if (!error.spec.empty())
        Dout(dc::continued, '\t' << error.spec << '\n');

      Dout(dc::finish, color_end);
      size_t real_object_count = error.objects.size();
      std::vector<uint64_t> handles(real_object_count);
      for (auto const& object : error.objects)
      {
        ASSERT(1 <= object.second.n && object.second.n <= real_object_count);
        handles[object.second.n - 1] = object.first;
      }
      for (int i = 0; i < real_object_count; ++i)
        Dout(dc::vkerror|dc::warning, "\e[30;43m" << (i + 1) << "\e[39;49m: " << static_cast<vk_defaults::DebugUtilsObjectNameInfoEXT>(error.objects[handles[i]].info));
      print_finish = false;
    }
    else
    {
      Dout(dc::vkerror|dc::warning|continued_cf, "\e[31m" << pCallbackData->pMessage);
      color_end = "\e[39;49m";
    }
  }
  else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    dcp = &DEBUGCHANNELS::dc::vkwarning;
    Dout(dc::vkwarning|dc::warning|continued_cf, "\e[31m" << pCallbackData->pMessage);
    color_end = "\e[39;49m";
  }
  else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    dcp = &DEBUGCHANNELS::dc::vkinfo;
    Dout(dc::vkinfo|continued_cf, pCallbackData->pMessage);
  }
  else
  {
    dcp = &DEBUGCHANNELS::dc::vkverbose;
    Dout(dc::vkverbose|continued_cf, pCallbackData->pMessage);
  }

  if (print_finish)
  {
    Dout(dc::finish, color_end);

    if (dcp->is_on() && pCallbackData->objectCount > 0)
    {
      Dout(*dcp, "With an objectCount of " << pCallbackData->objectCount << ":");
      for (int i = 0; i < pCallbackData->objectCount; ++i)
        Dout(*dcp, "\e[30;43m" << (i + 1) << "\e[39;49m: " << static_cast<vk_defaults::DebugUtilsObjectNameInfoEXT>(pCallbackData->pObjects[i]));
    }
  }

  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    if (NAMESPACE_DEBUG::being_traced())
      DoutFatal(dc::core, "Trap point");

    libcwd::libcw_do.pop_marker();
    libcwd::libcw_do.set_indent(indent);
  }

  return VK_FALSE;
}

} // namespace vulkan

#endif
