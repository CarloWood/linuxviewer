#include "sys.h"
#include "gui_Window.h"
//#include "gui_MenuBar.h"
#include "debug.h"

namespace glfw3 {
namespace gui {

Window::Window(Application* application) : m_application(application)
{
  DoutEntering(dc::notice, "Window::Window(" << application << ") [NOT IMPLEMENTED]");

  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);    // Default is TRUE.
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);      // Default is TRUE.
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);    // Default is TRUE.
  glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
  //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);  No effect?
  m_glfw_window = glfwCreateWindow(500, 800, "GUI", NULL, NULL);

  if (!m_glfw_window)
    throw std::runtime_error("Failed to create main window: glfwCreateWindow returned NULL");

  glfwMakeContextCurrent(m_glfw_window);

#if 0
  m_menubar = new MenuBar(this);
  m_grid.attach(*m_menubar, 0, 0);

  add(m_grid);

  show_all_children();
#endif
}

Window::~Window()
{
  Dout(dc::notice, "Calling Window::~Window()");
  glfwDestroyWindow(m_glfw_window);
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
