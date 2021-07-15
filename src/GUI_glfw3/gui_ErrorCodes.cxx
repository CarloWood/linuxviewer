#include "sys.h"
#include "GLFW/glfw3.h"
#include "utils/AIAlert.h"
#include "debug.h"
#include <string>
#include <sstream>

namespace glfw3 {

enum error_codes
{
  NOT_INITIALIZED       = GLFW_NOT_INITIALIZED,
  NO_CURRENT_CONTEXT    = GLFW_NO_CURRENT_CONTEXT,
  INVALID_ENUM          = GLFW_INVALID_ENUM,
  INVALID_VALUE         = GLFW_INVALID_VALUE,
  OUT_OF_MEMORY         = GLFW_OUT_OF_MEMORY,
  API_UNAVAILABLE       = GLFW_API_UNAVAILABLE,
  VERSION_UNAVAILABLE   = GLFW_VERSION_UNAVAILABLE,
  PLATFORM_ERROR        = GLFW_PLATFORM_ERROR,
  FORMAT_UNAVAILABLE    = GLFW_FORMAT_UNAVAILABLE,
  NO_WINDOW_CONTEXT     = GLFW_NO_WINDOW_CONTEXT,
  CURSOR_UNAVAILABLE    = GLFW_CURSOR_UNAVAILABLE,
  FEATURE_UNAVAILABLE   = GLFW_FEATURE_UNAVAILABLE,
  FEATURE_UNIMPLEMENTED = GLFW_FEATURE_UNIMPLEMENTED
};

std::error_code make_error_code(error_codes);

} // namespace glfw3

// Register glfw3::error_codes as valid error code.
namespace std {

template<> struct is_error_code_enum<glfw3::error_codes> : true_type { };

} // namespace std

namespace glfw3 {

// This function is more or less a copy of glfwpp/include/glfwpp/glfwpp.h.
void error_callback(int errorCode_, const char* what_)
{
  // This should never happen.
  ASSERT(errorCode_ != GLFW_NO_ERROR);
  if (errorCode_ < GLFW_NOT_INITIALIZED || errorCode_ > GLFW_FEATURE_UNIMPLEMENTED)
    DoutFatal(dc::core, "Unrecognized error GLFW error: 0x" << std::hex << errorCode_ << " (" << what_ << ")");

  error_codes error_code = static_cast<error_codes>(errorCode_);

  // Error handling philosophy as per http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0709r4.pdf (section 1.1)

  // Application programmer errors. See the GLFW docs and fix the code.
  ASSERT(error_code != NOT_INITIALIZED);
  ASSERT(error_code != NO_CURRENT_CONTEXT);
  ASSERT(error_code != NO_WINDOW_CONTEXT);
  ASSERT(error_code != INVALID_VALUE);

  // This error should never occur.
  ASSERT(error_code != INVALID_ENUM);

  // Allocation failure must be treated separately.
  if (error_code == OUT_OF_MEMORY)
    throw std::bad_alloc();

  // Runtime error.
  THROW_ALERTC(error_code, "glfw3::error_callback([ERRORCODE], \"[WHAT]\")", AIArgs("[ERRORCODE]", error_code)("[WHAT]", what_));
}

//============================================================================
// Error code handling.
// See https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/

//----------------------------------------------------------------------------
// glfw3 error category

namespace {

struct ErrorCategory : std::error_category
{
  char const* name() const noexcept override;
  std::string message(int ev) const override;
};

char const* ErrorCategory::name() const noexcept
{
  return "glfw3";
}

std::string ErrorCategory::message(int ev) const
{
  if (ev == GLFW_NO_ERROR)
    return "No error (GLFW_NO_ERROR)";
  switch (static_cast<error_codes>(ev))
  {
    case GLFW_NOT_INITIALIZED:
      return "The GLFW library is not initialized (GLFW_NOT_INITIALIZED)";
    case GLFW_NO_CURRENT_CONTEXT:
      return "There is no current context (GLFW_NO_CURRENT_CONTEXT)";
    case GLFW_INVALID_ENUM:
      return "Invalid argument for enum parameter (GLFW_INVALID_ENUM)";
    case GLFW_INVALID_VALUE:
      return "Invalid value for parameter (GLFW_INVALID_VALUE)";
    case GLFW_OUT_OF_MEMORY:
      return "Out of memory (GLFW_OUT_OF_MEMORY)";
    case GLFW_API_UNAVAILABLE:
      return "The requested API is unavailable (GLFW_API_UNAVAILABLE)";
    case GLFW_VERSION_UNAVAILABLE:
      return "The requested API version is unavailable (GLFW_VERSION_UNAVAILABLE)";
    case GLFW_PLATFORM_ERROR:
      return "A platform-specific error occurred (GLFW_PLATFORM_ERROR)";
    case GLFW_FORMAT_UNAVAILABLE:
      return "The requested format is unavailable (GLFW_FORMAT_UNAVAILABLE)";
    case GLFW_NO_WINDOW_CONTEXT:
      return "The specified window has no context (GLFW_NO_WINDOW_CONTEXT)";
    case GLFW_CURSOR_UNAVAILABLE:
      return "The specified cursor shape is unavailable (GLFW_CURSOR_UNAVAILABLE)";
    case GLFW_FEATURE_UNAVAILABLE:
      return "The requested feature cannot be implemented for this platform (GLFW_FEATURE_UNAVAILABLE)";
    case GLFW_FEATURE_UNIMPLEMENTED:
      return "The requested feature has not yet been implemented for this platform (GLFW_FEATURE_UNIMPLEMENTED)";
    default:
    {
      std::ostringstream oss;
      oss << "Unrecognized error (0x" << std::hex << ev << ")"; 
      return oss.str();
    }
  }
}

ErrorCategory const theErrorCategory { };

} // namespace

std::error_code make_error_code(error_codes code)
{
  return std::error_code(static_cast<int>(code), theErrorCategory);
}

} // namespace glfw3
