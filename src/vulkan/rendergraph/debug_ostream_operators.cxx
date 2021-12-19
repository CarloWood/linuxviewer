#include "sys.h"

#ifdef CWDEBUG
#include "debug_ostream_operators.h"
#include "RenderGraph.h"
#include "utils/macros.h"

namespace vulkan::rendergraph {

//static
std::string RenderGraph::to_string(Direction direction)
{
  switch (direction)
  {
    AI_CASE_RETURN(search_forwards);
    AI_CASE_RETURN(search_backwards);
  }
  return "<UNKNOWN DIRECTION TYPE>";
}

//static
std::string RenderPass::to_string(SearchType search_type)
{
  switch (search_type)
  {
    AI_CASE_RETURN(Subsequent);
    AI_CASE_RETURN(Outgoing);
    AI_CASE_RETURN(Incoming);
  }
  return "<UNKNOWN SEARCH TYPE>";
}

} // namespace vulkan::rendergraph
#endif
