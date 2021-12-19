#include "sys.h"
#include "LoadStoreOps.h"
#include "debug.h"
#ifdef CWDEBUG
#include "utils/AIAlert.h"
#endif

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct renderpass;
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace vulkan::rendergraph {

void LoadStoreOps::set_load()
{
#ifdef CWDEBUG
  // Don't mention an attachment twice in a modifier ([]) (aka: render_pass[~A][+A] <-- A twice).
  if ((m_mask & Clear))
    THROW_ALERT("Using CLEAR and LOAD at the same time");
  else if ((m_mask & Load))
    THROW_ALERT("Redundant LOAD");
#endif
  m_mask |= Load;
}

void LoadStoreOps::set_clear()
{
#ifdef CWDEBUG
  // Don't mention an attachment twice in a modifier ([]) (aka: render_pass[+A][~A] <-- A twice).
  // Don't clear an attachment twice (aka: render_pass[~A]->stores(~A) <-- cleared twice).
  // Don't clear an attachment that is specifically marked as a LOAD (aka: render_pass[+A]->stores(~A)).
  if ((m_mask & Load))
    THROW_ALERT("Using CLEAR and LOAD at the same time");
  else if ((m_mask & Clear))
    THROW_ALERT("Redundant CLEAR");
#endif
  m_mask |= Clear;
}

void LoadStoreOps::set_store()
{
  m_mask |= Store;
}

void LoadStoreOps::set_preserve()
{
  // Paranoia: Preserve should only be set on LD, or we'd get something illegal.
  ASSERT(m_mask == Load);
  m_mask |= Preserve;
}

} // namespace vulkan::rendergraph
