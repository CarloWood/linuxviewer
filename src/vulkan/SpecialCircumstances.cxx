#include "sys.h"
#include "SpecialCircumstances.h"
#ifdef CWDEBUG
#include "utils/macros.h"
#endif

namespace vulkan {

bool SpecialCircumstances::handled_map_changed(int map_flags)
{
  int new_map_flags;
  if (is_mapped(map_flags))
    new_map_flags = m_map_flags.fetch_and(AND_handle_MAPPED, std::memory_order::relaxed) & AND_handle_MAPPED;
  else
    new_map_flags = m_map_flags.fetch_or(OR_handle_UNMAPPED, std::memory_order::relaxed) | OR_handle_UNMAPPED;
  // If the above atomic operation did nothing, then the calling thread still has an event to handle.
  bool no_change = new_map_flags == unhandled_UNMAPPED || new_map_flags == unhandled_MAPPED;
  if (no_change)
    m_flags.fetch_or(map_changed_bit, std::memory_order::release);
  return !no_change;
}

void SpecialCircumstances::set_mapped() const
{
  if (is_mapped())    // Should never happen - but we don't control what the X server is sending us.
    return;
  m_map_flags.fetch_or(OR_UNMAPPED_to_MAPPED, std::memory_order::relaxed);
  m_flags.fetch_or(map_changed_bit, std::memory_order::release);              // Make m_map_flags available.
}

void SpecialCircumstances::set_unmapped() const
{
  if (!is_mapped())   // Should never happen - but we don't control what the X server is sending us.
    return;
  m_map_flags.fetch_and(AND_MAPPED_to_UNMAPPED, std::memory_order::relaxed);
  m_flags.fetch_or(map_changed_bit, std::memory_order::release);              // Make m_map_flags available.
}

#ifdef CWDEBUG
//static
char const* SpecialCircumstances::print_map_flags(int map_flags)
{
  switch (map_flags)
  {
    AI_CASE_RETURN(unhandled_UNMAPPED);
    AI_CASE_RETURN(handled_UNMAPPED);
    AI_CASE_RETURN(handled_MAPPED);
    AI_CASE_RETURN(unhandled_MAPPED);
  }
  return "<corrupt map flags>";
}
#endif

} // namespace vulkan

