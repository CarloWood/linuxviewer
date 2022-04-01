#include "sys.h"
#include "Directories.h"
#include "Defaults.h"
#include "peelo/xdg.hpp"
#include "utils/AIAlert.h"
#include "utils/macros.h"
#include <algorithm>
#include <iterator>
#include <cctype>
#include <iostream>
#include "debug.h"

namespace vulkan {

void Directories::set_application_name(std::u8string const& application_name)
{
  // On linux any character is possible in a filename except '/'.
  // On windows a whole host is illegal, but so are names that exist of just legal characters.
  // To keep things simple (and readable) I'm just going to allow only characters
  // from the POSIX portable filename character set (https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_282),
  // plus all UTF8 characters.
  //
  // However, as first character we only accept alpha.
  // Moreover, all uppercase characters are converted to lowercase.

  constexpr char delete_char = '/';
  std::string result;
  bool first_character = true;
  bool last_was_lower = false;
  for (char c : application_name)
  {
    // Accept all UTF8 characters.
    if ((c & 0x80))
    {
      result += c;
      continue;
    }
    if (first_character)
    {
      if (std::isalpha(c))
      {
        first_character = false;
        result += std::tolower(c);
      }
      // Leading characters that are not alpha are just trimmed off.
    }
    else if (std::isalnum(c) || c == '.' || c == '_' || c == '-')
    {
      if (std::isupper(c) && last_was_lower)
        result += '_';
      result += std::tolower(c);
      last_was_lower = std::islower(c);
    }
    else
    {
      last_was_lower = false;
      result += std::isspace(c) ? '-' : '_';
    }
    continue;
  }
  auto pos = result.find_last_of(delete_char);
  m_application_name = result.substr(pos + 1);
}

void Directories::initialize(std::u8string const& application_name, std::filesystem::path executable_path)
{
  DoutEntering(dc::notice, "Directories::initialize(\"" << application_name << "\", " << executable_path << ")");

  // The derived class must override:
  //   std::u8string application_name() const override { return u8"My Cool Application"; }
  if (application_name == reinterpret_cast<char8_t const*>(vk_defaults::ApplicationInfo::default_application_name))
    THROW_ALERT("Application name must be unique; it will be used as directory name to store application specific files.");

  set_application_name(application_name);
  Dout(dc::notice, "Application directory name: " << m_application_name << ".");

  // Get the directory that the executable resides in.
  auto directory_element_iterator = executable_path.end();
  int n = 0;
  m_installed = false;
  while (directory_element_iterator != executable_path.begin())
  {
    --directory_element_iterator;
    if (++n == 2)
    {
      m_installed = *directory_element_iterator == "bin";
      break;
    }
  }

  m_executable_name = executable_path.filename();
  std::filesystem::path install_prefix = executable_path.parent_path();
  if (m_installed)
    install_prefix = install_prefix.parent_path();      // Also remove "/bin".

  for (int i = 0; i < number_of_directories; ++i)
  {
    Directory directory = static_cast<Directory>(i);
    std::optional<std::filesystem::path> dir;
    switch (directory)
    {
      case Directory::data:
        dir = peelo::xdg::data_dir();
        break;
      case Directory::config:
        dir = peelo::xdg::config_dir();
        break;
      case Directory::state:
        dir = peelo::xdg::state_dir();
        break;
      case Directory::cache:
        dir = peelo::xdg::cache_dir();
        break;
      case Directory::runtime:
        dir = peelo::xdg::runtime_dir();
        break;
      case Directory::prefix:
        dir = install_prefix;
        break;
    }
    if (!dir.has_value())
      THROW_ALERT("Could not find XDG base directory for [DIRECTORY]", AIArgs("[DIRECTORY]", directory));
    m_paths[i] = dir.value();
    if (directory <= Directory::runtime)
      m_paths[i] /= m_application_name;
    Dout(dc::notice, directory << " = " << m_paths[i]);
  }

#ifdef CWDEBUG
  Dout(dc::notice, "All XDG data dirs:");
  {
    NAMESPACE_DEBUG::Mark mark;
    auto data_dirs = peelo::xdg::all_data_dirs();
    for (auto dir : data_dirs)
      Dout(dc::notice, dir);
  }
  Dout(dc::notice, "All XDG config dirs:");
  {
    NAMESPACE_DEBUG::Mark mark;
    auto config_dirs = peelo::xdg::all_config_dirs();
    for (auto dir : config_dirs)
      Dout(dc::notice, dir);
  }
  Dout(dc::notice, "resources_path() = " << path_of(Directory::resources));
#endif
}

std::filesystem::path Directories::path_of(Directory directory) const
{
  std::filesystem::path basedir;
  if (directory < Directory::resources)
    basedir = m_paths[directory];
  else if (directory == Directory::resources)
  {
    basedir = m_installed ? m_paths[Directory::prefix] / "share" / m_executable_name / "resources"
                          : m_paths[Directory::prefix] / "data";
  }
  else
    THROW_ALERT("Unknown Directory ", AIArgs("[DIRECTORY]", directory));
  if (!std::filesystem::exists(basedir))
  {
    Dout(dc::warning, "The directory " << basedir << " does not exist. Trying to create it...");
    std::filesystem::create_directories(basedir);
  }
  return basedir;
}

char const* to_string(Directory dir)
{
  switch (dir)
  {
    AI_CASE_RETURN(Directory::data);
    AI_CASE_RETURN(Directory::config);
    AI_CASE_RETURN(Directory::state);
    AI_CASE_RETURN(Directory::cache);
    AI_CASE_RETURN(Directory::runtime);
    AI_CASE_RETURN(Directory::prefix);
  }
  AI_NEVER_REACHED
}

std::ostream& operator<<(std::ostream& os, Directory dir)
{
  return os << to_string(dir);
}

} // namespace vulkan
