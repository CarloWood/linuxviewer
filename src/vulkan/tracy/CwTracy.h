#pragma once

#include <Tracy.hpp>
#include <TracyVulkan.hpp>

#ifdef TRACY_ENABLE
#include "SourceLocationDataIterator.h"

// Used to pass comma's.
#define CW_TRACY_ADD(...) __VA_ARGS__

#ifndef TRACY_HAS_CALLSTACK
#define IF_TRACY_HAS_CALLSTACK_USE(depth)
#define COMMA_TRACY_CALLSTACK_IF_WE_HAVE_IT
#elif defined(TRACY_CALLSTACK)
#define IF_TRACY_HAS_CALLSTACK_USE(depth) depth
#define COMMA_TRACY_CALLSTACK_IF_WE_HAVE_IT , TRACY_CALLSTACK
#else
#define IF_TRACY_HAS_CALLSTACK_USE(depth) depth
#define COMMA_TRACY_CALLSTACK_IF_WE_HAVE_IT
#endif

#define CwTracyVkNamedZoneCOptS(ctx, varname, cmdbuf, name, color, depth, active, number_of_indices, current_index) \
  static utils::Vector<tracy::SourceLocationData, decltype(number_of_indices)> TracyConcat(__tracy_gpu_source_location, __LINE__){ \
    tracy::SourceLocationDataIterator<decltype(number_of_indices)>(name, __FUNCTION__,  __FILE__, __LINE__, color), \
    tracy::SourceLocationDataIterator(number_of_indices) }; \
  tracy::VkCtxScope varname(ctx, &TracyConcat(__tracy_gpu_source_location, __LINE__)[current_index], cmdbuf IF_TRACY_HAS_CALLSTACK_USE(depth), active)

// S means the user specified a depth that we should use (if TRACY_HAS_CALLSTACK is defined).
#define CwTracyVkNamedZoneCS(   ctx, varname, cmdbuf, name, color, depth,                 active, number_of_indices, current_index) \
        CwTracyVkNamedZoneCOptS(ctx, varname, cmdbuf, name, color, CW_TRACY_ADD(, depth), active, number_of_indices, current_index)

// No S means we should use TRACY_CALLSTACK as depth - provided TRACY_CALLSTACK (and TRACY_HAS_CALLSTACK) is defined.
#define CwTracyVkNamedZoneC(    ctx, varname, cmdbuf, name, color,                                                    active, number_of_indices, current_index) \
        CwTracyVkNamedZoneCOptS(ctx, varname, cmdbuf, name, color, CW_TRACY_ADD(COMMA_TRACY_CALLSTACK_IF_WE_HAVE_IT), active, number_of_indices, current_index)

// No C means the color is 0.
#define CwTracyVkNamedZoneS( ctx, varname, cmdbuf, name,    depth, active, number_of_indices, current_index) \
        CwTracyVkNamedZoneCS(ctx, varname, cmdbuf, name, 0, depth, active, number_of_indices, current_index)
#define CwTracyVkNamedZone(  ctx, varname, cmdbuf, name,           active, number_of_indices, current_index) \
        CwTracyVkNamedZoneC( ctx, varname, cmdbuf, name, 0,        active, number_of_indices, current_index)

// If the user doesn't specify a varname, we have to provide one. These are also always active.
#define CwTracyVkZoneCS(ctx,                    cmdbuf, name, color, depth,       number_of_indices, current_index) \
   CwTracyVkNamedZoneCS(ctx, ___tracy_gpu_zone, cmdbuf, name, color, depth, true, number_of_indices, current_index)
#define CwTracyVkZoneC( ctx,                    cmdbuf, name, color,              number_of_indices, current_index) \
   CwTracyVkNamedZoneC( ctx, ___tracy_gpu_zone, cmdbuf, name, color,        true, number_of_indices, current_index)
#define CwTracyVkZoneS( ctx,                    cmdbuf, name,        depth,       number_of_indices, current_index) \
   CwTracyVkNamedZoneS( ctx, ___tracy_gpu_zone, cmdbuf, name,        depth, true, number_of_indices, current_index)
#define CwTracyVkZone(  ctx,                    cmdbuf, name,                     number_of_indices, current_index) \
   CwTracyVkNamedZone(  ctx, ___tracy_gpu_zone, cmdbuf, name,               true, number_of_indices, current_index)


#define CwZoneNamedNCOptS(varname, name, color, depth, active, number_of_indices, current_index) \
  static utils::Vector<tracy::SourceLocationData, decltype(number_of_indices)> TracyConcat(__tracy_source_location, __LINE__){ \
    tracy::SourceLocationDataIterator<decltype(number_of_indices)>(name, __FUNCTION__,  __FILE__, __LINE__, color), \
    tracy::SourceLocationDataIterator(number_of_indices) }; \
  tracy::ScopedZone varname(&TracyConcat(__tracy_source_location, __LINE__)[current_index] IF_TRACY_HAS_CALLSTACK_USE(depth), active)

// S means the user specified a depth that we should use (if TRACY_HAS_CALLSTACK is defined).
#define CwZoneNamedNCS(   varname, name, color, depth,                 active, number_of_indices, current_index) \
        CwZoneNamedNCOptS(varname, name, color, CW_TRACY_ADD(, depth), active, number_of_indices, current_index)

// No S means we should use TRACY_CALLSTACK as depth - provided TRACY_CALLSTACK (and TRACY_HAS_CALLSTACK) is defined.
#define CwZoneNamedNC(    varname, name, color,                                                    active, number_of_indices, current_index) \
        CwZoneNamedNCOptS(varname, name, color, CW_TRACY_ADD(COMMA_TRACY_CALLSTACK_IF_WE_HAVE_IT), active, number_of_indices, current_index)

// No C means the color is 0.
#define CwZoneNamedNS( varname, name,    depth, active, number_of_indices, current_index) \
        CwZoneNamedNCS(varname, name, 0, depth, active, number_of_indices, current_index)
#define CwZoneNamedN(  varname, name,           active, number_of_indices, current_index) \
        CwZoneNamedNC( varname, name, 0,        active, number_of_indices, current_index)

// If the user doesn't specify a varname, we have to provide one. These are also always active.
#define CwZoneScopedNCS(                      name, color, depth,       number_of_indices, current_index) \
         CwZoneNamedNCS(___tracy_scoped_zone, name, color, depth, true, number_of_indices, current_index)
#define CwZoneScopedNC(                       name, color,              number_of_indices, current_index) \
         CwZoneNamedNC( ___tracy_scoped_zone, name, color,        true, number_of_indices, current_index)
#define CwZoneScopedNS(                       name,        depth,       number_of_indices, current_index) \
         CwZoneNamedNS( ___tracy_scoped_zone, name,        depth, true, number_of_indices, current_index)
#define CwZoneScopedN(                        name,                     number_of_indices, current_index) \
         CwZoneNamedN(  ___tracy_scoped_zone, name,               true, number_of_indices, current_index)


// Helper functions and struct for CwTracyCZoneNCOptS.
namespace tracy {

inline TracyCZoneCtx cw_emit_zone_begin(struct ___tracy_source_location_data const* srcloc, int active)
{
  return ___tracy_emit_zone_begin(srcloc, active);
}

inline TracyCZoneCtx cw_emit_zone_begin(struct ___tracy_source_location_data const* srcloc, int depth, int active)
{
  return ___tracy_emit_zone_begin_callstack(srcloc, depth, active);
}

struct CSourceLocationData : public ___tracy_source_location_data
{
  CSourceLocationData(SourceLocationData const& data) :
    ___tracy_source_location_data{.name = data.name, .function = data.function, .file = data.file, .line = data.line, .color = data.color} { }
};

} // namespace tracy

#define CwTracyCZoneNCOptS(ctx, name, color, depth, active, number_of_indices, current_index) \
  static utils::Vector<tracy::CSourceLocationData, decltype(number_of_indices)> const TracyConcat(__tracy_source_location, __LINE__){ \
    tracy::SourceLocationDataIterator<decltype(number_of_indices)>(name, __func__,  __FILE__, __LINE__, color), \
    tracy::SourceLocationDataIterator(number_of_indices) }; \
  TracyCZoneCtx ctx = tracy::cw_emit_zone_begin(&TracyConcat(__tracy_source_location, __LINE__)[current_index] IF_TRACY_HAS_CALLSTACK_USE(depth), active )

#define CwTracyCZoneNCS(   ctx, name, color, depth,                 active, number_of_indices, current_index) \
        CwTracyCZoneNCOptS(ctx, name, color, CW_TRACY_ADD(, depth), active, number_of_indices, current_index)

#define CwTracyCZoneNC(    ctx, name, color,                                                    active, number_of_indices, current_index) \
        CwTracyCZoneNCOptS(ctx, name, color, CW_TRACY_ADD(COMMA_TRACY_CALLSTACK_IF_WE_HAVE_IT), active, number_of_indices, current_index)

#define CwTracyCZoneNS( ctx, name,    depth, active, number_of_indices, current_index) \
        CwTracyCZoneNCS(ctx, name, 0, depth, active, number_of_indices, current_index)
#define CwTracyCZoneN(  ctx, name,           active, number_of_indices, current_index) \
        CwTracyCZoneNC( ctx, name, 0,        active, number_of_indices, current_index)

// No N means the name is nullptr.
#define CwTracyCZoneCS( ctx,          color, depth, active, number_of_indices, current_index) \
        CwTracyCZoneNCS(ctx, nullptr, color, depth, active, number_of_indices, current_index)
#define CwTracyCZoneC(  ctx,          color,        active, number_of_indices, current_index) \
        CwTracyCZoneNC( ctx, nullptr, color,        active, number_of_indices, current_index)
#define CwTracyCZoneS(  ctx,                 depth, active, number_of_indices, current_index) \
        CwTracyCZoneNS( ctx, nullptr,        depth, active, number_of_indices, current_index)
#define CwTracyCZone(   ctx,                        active, number_of_indices, current_index) \
        CwTracyCZoneN(  ctx, nullptr,               active, number_of_indices, current_index)

#else

#define CwTracyVkNamedZoneCS(ctx, varname, cmdbuf, name, color, depth, active, number_of_indices, current_index)
#define CwTracyVkNamedZoneC(ctx, varname, cmdbuf, name, color, active, number_of_indices, current_index)
#define CwTracyVkNamedZoneS(ctx, varname, cmdbuf, name, depth, active, number_of_indices, current_index)
#define CwTracyVkNamedZone(ctx, varname, cmdbuf, name, active, number_of_indices, current_index)
#define CwTracyVkZoneCS(ctx, cmdbuf, name, color, depth, number_of_indices, current_index)
#define CwTracyVkZoneC(ctx, cmdbuf, name, color, number_of_indices, current_index)
#define CwTracyVkZoneS(ctx, cmdbuf, name, depth, number_of_indices, current_index)
#define CwTracyVkZone(ctx, cmdbuf, name, number_of_indices, current_index)

#define CwZoneNamedNCS(varname, name, color, depth, active, number_of_indices, current_index)
#define CwZoneNamedNC(varname, name, color, active, number_of_indices, current_index)
#define CwZoneNamedNS(varname, name, depth, active, number_of_indices, current_index)
#define CwZoneNamedN(varname, name, active, number_of_indices, current_index)
#define CwZoneScopedNCS(name, color, depth, number_of_indices, current_index)
#define CwZoneScopedNC(name, color, number_of_indices, current_index)
#define CwZoneScopedNS(name, depth, number_of_indices, current_index)
#define CwZoneScopedN(name, number_of_indices, current_index)

#define CwTracyCZoneNCS(ctx, name, color, depth, active, number_of_indices, current_index)
#define CwTracyCZoneNC(ctx, name, color, active, number_of_indices, current_index)
#define CwTracyCZoneNS(ctx, name, depth, active, number_of_indices, current_index)
#define CwTracyCZoneN(ctx, name, active, number_of_indices, current_index)
#define CwTracyCZoneCS(ctx, color, depth, active, number_of_indices, current_index)
#define CwTracyCZoneC(ctx, color, active, number_of_indices, current_index)
#define CwTracyCZoneS(ctx, depth, active, number_of_indices, current_index)
#define CwTracyCZone(ctx, active, number_of_indices, current_index)

#endif
