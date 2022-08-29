#include "sys.h"
#include "Window.h"

Window::~Window()
{
  // LogicalDevice will destroy the pool.
  m_top_descriptor_set.release();
  m_left_descriptor_set.release();
  m_bottom_descriptor_set.release();
}
