#pragma once

#include <filesystem>
#include <iosfwd>

namespace vulkan {

// From https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html:
//
// $XDG_DATA_HOME defines the base directory relative to which user-specific data files should be stored.
// If $XDG_DATA_HOME is either not set or empty, a default equal to $HOME/.local/share should be used.
//
// $XDG_CONFIG_HOME defines the base directory relative to which user-specific configuration files should be stored.
// If $XDG_CONFIG_HOME is either not set or empty, a default equal to $HOME/.config should be used.
//
// $XDG_STATE_HOME defines the base directory relative to which user-specific state files should be stored.
// If $XDG_STATE_HOME is either not set or empty, a default equal to $HOME/.local/state should be used.
//
//      The $XDG_STATE_HOME contains state data that should persist between (application) restarts,
//      but that is not important or portable enough to the user that it should be stored in $XDG_DATA_HOME.
//      It may contain:
//
//      actions history (logs, history, recently used files, …)
//
//      current state of the application that can be reused on a restart (view, layout, open files, undo history, …)
//
// $XDG_CACHE_HOME defines the base directory relative to which user-specific non-essential data files should be stored.
// If $XDG_CACHE_HOME is either not set or empty, a default equal to $HOME/.cache should be used.
//
// $XDG_RUNTIME_DIR defines the base directory relative to which user-specific non-essential runtime files
// and other file objects (such as sockets, named pipes, ...) should be stored.
//
//      The directory MUST be owned by the user, and he MUST be the only one having read and write access to it.
//      Its Unix access mode MUST be 0700.
//
//      The lifetime of the directory MUST be bound to the user being logged in. It MUST be created when the
//      user first logs in and if the user fully logs out the directory MUST be removed. If the user logs in
//      more than once he should get pointed to the same directory, and it is mandatory that the directory
//      continues to exist from his first login to his last logout on the system, and not removed in between.
//      Files in the directory MUST not survive reboot or a full logout/login cycle.
//
//      The directory MUST be on a local file system and not shared with any other system. The directory MUST
//      be fully-featured by the standards of the operating system. More specifically, on Unix-like operating
//      systems AF_UNIX sockets, symbolic links, hard links, proper permissions, file locking, sparse files,
//      memory mapping, file change notifications, a reliable hard link count must be supported, and no
//      restrictions on the file name character set should be imposed. Files in this directory MAY be subjected
//      to periodic clean-up. To ensure that your files are not removed, they should have their access time
//      timestamp modified at least once every 6 hours of monotonic time or the 'sticky' bit should be set on
//      the file.
//
//      If $XDG_RUNTIME_DIR is not set applications should fall back to a replacement directory with similar
//      capabilities and print a warning message. Applications should use this directory for communication
//      and synchronization purposes and should not place larger files in it, since it might reside in
//      runtime memory and cannot necessarily be swapped out to disk.
//
struct Directory
{
  // Stores in Directories::m_path.
  static constexpr int data = 0;       // user-specific data files
  static constexpr int config = 1;     // user-specific configuration files
  static constexpr int state = 2;      // user-specific state data
  static constexpr int cache = 3;      // user-specific non-essential (cached) data
  static constexpr int runtime = 4;    // user-specific runtime files and other file objects
  static constexpr int prefix = 5;     // Install prefix (ie, /usr or /usr/local), or set to the build directory when not installed.

  // Other directories.
  static constexpr int resources = 6;

  int m_val;

  Directory(int val) : m_val(val) { }
  Directory(Directory const& other) : m_val(other.m_val) { }

  operator int() const { return m_val; }
};

class Directories
{
 private:
  static constexpr int number_of_directories = static_cast<int>(Directory::prefix) + 1;
  std::array<std::filesystem::path, number_of_directories> m_paths;
  std::filesystem::path m_application_name;                             // Application unique identifier that can be used as a directory name.
  std::filesystem::path m_executable_name;                              // The filename component of argv[0].
  bool m_installed;                                                     // True when the running executable was installed in a some <prefix>/bin directory.

 private:
  void set_application_name(std::u8string const& application_name);

 public:
  Directories() = default;

  void initialize(std::u8string const& application_name, std::filesystem::path executable_path);

  std::filesystem::path path_of(Directory directory) const;
};

std::ostream& operator<<(std::ostream& os, Directory dir);

} // namespace vulkan
