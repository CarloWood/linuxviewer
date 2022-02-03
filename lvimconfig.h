#pragma once

// Use a thread-local ImGui context.
struct ImGuiContext;
extern thread_local ImGuiContext* lvImGuiTLS;
#define GImGui lvImGuiTLS

#include "debug.h"
#define IM_ASSERT(_EXPR) ASSERT(_EXPR)

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define IMGUI_DISABLE_OBSOLETE_KEYIO
