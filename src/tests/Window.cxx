#include "sys.h"
#include "Window.h"

vulkan::ImageKind const Window::s_vector_image_kind{{ .format = vk::Format::eR16G16B16A16Sfloat }};
vulkan::ImageViewKind const Window::s_vector_image_view_kind{s_vector_image_kind, {}};
