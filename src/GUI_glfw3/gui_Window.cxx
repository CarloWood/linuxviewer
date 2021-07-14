#include "sys.h"
#include "gui_Window.h"
#include "gui_Application.h"
//#include "gui_MenuBar.h"
#include "debug.h"

namespace glfw3 {
namespace gui {

Window::Window(Application* application, int width, int height, char const* title, glfw::Monitor const* monitor, glfw::Window const* share) :
  glfw::Window(width, height, title ? title : application->application_name().c_str(), monitor, share), m_application(application)
{
  DoutEntering(dc::notice, "Window::Window(" << application << ", " << width << ", " << height << ", \"" << title << "\", " << monitor << ", " << share << ") [" << (void*)this << "] [INCOMPLETE]");

#if 0
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);    // Default is TRUE.
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);      // Default is TRUE.
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);    // Default is TRUE.
  glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
  //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);  No effect?
  m_glfw_window = glfwCreateWindow(500, 800, "GUI", NULL, NULL);

  if (!m_glfw_window)
    throw std::runtime_error("Failed to create main window: glfwCreateWindow returned NULL");
#endif

#if 0
  // FIXME: is this still possible now that we do glfw::makeContextCurrent() after returning from this constructor?
  // Maybe this should be delayed till after that.

  m_menubar = new MenuBar(this);
  m_grid.attach(*m_menubar, 0, 0);

  add(m_grid);

  show_all_children();
#endif
}

Window::~Window()
{
  Dout(dc::notice, "Calling Window::~Window() [" << (void*)this << "]");
}

void Window::append_menu_entries(MenuBar* menubar)
{
  DoutEntering(dc::notice, "Window::append_menu_entries(" << (void*)menubar << ") [NOT IMPLEMENTED]");

#if 0
  using namespace menu_keys;
  using namespace Gtk::Stock;
#define ADD(top, entry) \
  menubar->append_menu_entry({top, entry},   this, &Window::on_menu_##top##_##entry)

  Gtk::RadioButtonGroup mode;
#define ADD_RADIO(top, entry) \
  menubar->append_radio_menu_entry(mode, {top, entry},   this, &Window::on_menu_##top##_##entry)
#endif

//  ADD(File, OPEN);
}

} // namespace gui
} // namespace glfw3
