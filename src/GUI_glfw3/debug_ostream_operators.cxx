#include "sys.h"
#include "debug_ostream_operators.h"
#include "utils/macros.h"
#include <iostream>
#include <iomanip>

namespace glfw3 {
namespace gui {

template<typename T>
char const* print_enum(T en)
{
  return "UNKNOWN ENUM";
}

template<>
char const* print_enum(glfw::ClientApi en)
{
  using namespace glfw;
  switch (en)
  {
    AI_CASE_RETURN(ClientApi::OpenGl);
    AI_CASE_RETURN(ClientApi::OpenGles);
    AI_CASE_RETURN(ClientApi::None);
  }
}

template<>
char const* print_enum(glfw::ContextCreationApi en)
{
  using namespace glfw;
  switch (en)
  {
    AI_CASE_RETURN(ContextCreationApi::Native);
    AI_CASE_RETURN(ContextCreationApi::Egl);
    AI_CASE_RETURN(ContextCreationApi::OsMesa);
  }
}

template<>
char const* print_enum(glfw::ContextRobustness en)
{
  using namespace glfw;
  switch (en)
  {
    AI_CASE_RETURN(ContextRobustness::NoRobustness);
    AI_CASE_RETURN(ContextRobustness::NoResetNotification);
    AI_CASE_RETURN(ContextRobustness::LoseContextOnReset);
  }
}

template<>
char const* print_enum(glfw::ContextReleaseBehavior en)
{
  using namespace glfw;
  switch (en)
  {
    AI_CASE_RETURN(ContextReleaseBehavior::Any);
    AI_CASE_RETURN(ContextReleaseBehavior::Flush);
    AI_CASE_RETURN(ContextReleaseBehavior::None);
  }
}

template<>
char const* print_enum(glfw::OpenGlProfile en)
{
  using namespace glfw;
  switch (en)
  {
    AI_CASE_RETURN(OpenGlProfile::Any);
    AI_CASE_RETURN(OpenGlProfile::Compat);
    AI_CASE_RETURN(OpenGlProfile::Core);
  }
}

std::ostream& operator<<(std::ostream& os, glfw::WindowHints const& window_hints)
{
  os << std::boolalpha << '{';
  os << "resizable:" << window_hints.resizable << ", ";
  os << "visible:" << window_hints.visible << ", ";
  os << "decorated:" << window_hints.decorated << ", ";
  os << "focused:" << window_hints.focused << ", ";
  os << "autoIconify:" << window_hints.autoIconify << ", ";
  os << "floating:" << window_hints.floating << ", ";
  os << "maximized:" << window_hints.maximized << ", ";
  os << "centerCursor:" << window_hints.centerCursor << ", ";
  os << "transparentFramebuffer:" << window_hints.transparentFramebuffer << ", ";
  os << "focusOnShow:" << window_hints.focusOnShow << ", ";
  os << "scaleToMonitor:" << window_hints.scaleToMonitor << ", ";
  os << "redBits:" << window_hints.redBits << ", ";
  os << "greenBits:" << window_hints.greenBits << ", ";
  os << "blueBits:" << window_hints.blueBits << ", ";
  os << "alphaBits:" << window_hints.alphaBits << ", ";
  os << "depthBits:" << window_hints.depthBits << ", ";
  os << "stencilBits:" << window_hints.stencilBits << ", ";
  os << "accumRedBits:" << window_hints.accumRedBits << ", ";
  os << "accumGreenBits:" << window_hints.accumGreenBits << ", ";
  os << "accumBlueBits:" << window_hints.accumBlueBits << ", ";
  os << "accumAlphaBits:" << window_hints.accumAlphaBits << ", ";
  os << "auxBuffers:" << window_hints.auxBuffers << ", ";
  os << "samples:" << window_hints.samples << ", ";
  os << "refreshRate:" << window_hints.refreshRate << ", ";
  os << "stereo:" << window_hints.stereo << ", ";
  os << "srgbCapable:" << window_hints.srgbCapable << ", ";
  os << "doubleBuffer:" << window_hints.doubleBuffer << ", ";
  os << "clientApi:" << print_enum(window_hints.clientApi) << ", ";
  os << "contextCreationApi:" << print_enum(window_hints.contextCreationApi) << ", ";
  os << "contextVersionMajor:" << window_hints.contextVersionMajor << ", ";
  os << "contextVersionMinor:" << window_hints.contextVersionMinor << ", ";
  os << "contextRobustness:" << print_enum(window_hints.contextRobustness) << ", ";
  os << "contextReleaseBehavior:" << print_enum(window_hints.contextReleaseBehavior) << ", ";
  os << "openglForwardCompat:" << window_hints.openglForwardCompat << ", ";
  os << "openglDebugContext:" << window_hints.openglDebugContext << ", ";
  os << "openglProfile:" << print_enum(window_hints.openglProfile) << ", ";
  os << "cocoaRetinaFramebuffer:" << window_hints.cocoaRetinaFramebuffer << ", ";
  os << "cocoaFrameName:\"" << window_hints.cocoaFrameName << "\", ";
  os << "cocoaGraphicsSwitching:" << window_hints.cocoaGraphicsSwitching << ", ";
  os << "x11ClassName:\"" << window_hints.x11ClassName << "\", ";
  os << "x11InstanceName:\"" << window_hints.x11InstanceName;
  os << '}';
  return os;
}

} // namespace glfw3
} // namespace gui
