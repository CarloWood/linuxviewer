#pragma once

#include <cstdint>
#include <iosfwd>

namespace vulkan::rendergraph {

class LoadStoreOps
{
 private:
  uint32_t m_mask = {};

 public:
  static constexpr uint32_t Load = 1;
  static constexpr uint32_t Clear = 2;
  static constexpr uint32_t Store = 4;
  static constexpr uint32_t Preserve = 8;

  // The seven legal values are:
  // DD : none          = 0
  // DS : Store         = 4
  // CD : Clear         = 2
  // CS : Clear|Store   = 6
  // LD : Load          = 1
  // LS : Load|Store    = 5
  // LP : Load|Preserve = 9

  void set_load();
  void set_clear();
  void set_store();
  void set_preserve();

  bool is_load() const { return m_mask & Load; }
  bool is_clear() const { return m_mask & Clear; }
  bool is_store() const { return m_mask & Store; }
  bool is_preserve() const { return m_mask & Preserve; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::rendergraph
