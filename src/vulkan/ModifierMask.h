#include <iosfwd>
#include <cstdint>

namespace vulkan {

struct ModifierMask
{
  static constexpr int Shift   = 1 << 0;        // Both, left and/or right.
  static constexpr int Lock    = 1 << 1;        // When pressed or CapsLock on.
  static constexpr int Ctrl    = 1 << 2;        // Both, left and/or right.
  static constexpr int Alt     = 1 << 3;        // Both, left and/or right.
  static constexpr int Super   = 1 << 4;

 private:
  uint16_t m_mask;

 public:
  ModifierMask() : m_mask(0) { }
  ModifierMask(uint16_t mask) : m_mask(mask) { }
  ModifierMask(ModifierMask const& orig) : m_mask(orig.m_mask) { }

  std::string to_string() const;
};

std::ostream& operator<<(std::ostream& os, ModifierMask mask);

} // namespace vulkan
