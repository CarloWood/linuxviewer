#include "sys.h"
#include "LinuxViewerApplication.h"
#include "LinuxViewerMenuBar.h"
#include "protocols/GridInfoDecoder.h"
#include "protocols/GridInfo.h"
#include "statefultask/AIEngine.h"
#include "socket-task/ConnectToEndPoint.h"
#include "evio/Socket.h"
#include "evio/File.h"
#include "evio/protocol/http.h"
#include "evio/protocol/EOFDecoder.h"
#include "protocols/LlsdDecoder.h"
#include <functional>

namespace http = evio::protocol::http;

//static
std::unique_ptr<LinuxViewerApplication> LinuxViewerApplication::create(AIEngine& gui_idle_engine)
{
  return std::make_unique<LinuxViewerApplication>(gui_idle_engine);
}

class MySocket : public evio::Socket
{
 private:
  http::ResponseHeadersDecoder m_input_decoder;
  GridInfoDecoder m_grid_info_decoder;
  LlsdDecoder m_llsd_decoder;
  evio::OutputStream m_output_stream;
  GridInfo m_grid_info;

 public:
  MySocket() :
    m_input_decoder(
        {{"application/xml", m_grid_info_decoder},
         {"text/xml", m_llsd_decoder}}
        ),
    m_grid_info_decoder(m_grid_info)
  {
    m_grid_info_decoder.set_next_decoder(evio::protocol::EOFDecoder::instance(), [this](){ return m_input_decoder.content_length(); });
    m_llsd_decoder.set_next_decoder(evio::protocol::EOFDecoder::instance(), [this](){ return m_input_decoder.content_length(); });
    set_protocol_decoder(m_input_decoder);
    set_source(m_output_stream);
  }

  evio::OutputStream& output_stream() { return m_output_stream; }
};

class MyFile : public evio::File
{
 private:
  boost::intrusive_ptr<MySocket> m_linked_output_device;

 public:
  MyFile(boost::intrusive_ptr<MySocket> linked_output_device) : m_linked_output_device(std::move(linked_output_device)) { }

  // If the file is closed we wrote everything, so call flush on the socket.
  void closed(int& UNUSED_ARG(allow_deletion_count)) override { m_linked_output_device->flush_output_device(); }
};

// Called when the main instance (as determined by the GUI) of the application is starting.
void LinuxViewerApplication::on_main_instance_startup()
{
  // Run a test task.
  boost::intrusive_ptr<task::ConnectToEndPoint> task = new task::ConnectToEndPoint(CWDEBUG_ONLY(true));
  auto socket = evio::create<MySocket>();
  task->set_socket(socket);
  task->set_end_point(AIEndPoint("misfitzgrid.com", 8002));
  task->on_connected([socket](bool success){
      if (success)
      {
#if 0
        socket->output_stream() << "GET /get_grid_info HTTP/1.1\r\n"
                                   "Host: misfitzgrid.com:8002\r\n"
                                   "Accept-Encoding:\r\n"
                                   "Accept: application/xml\r\n"
                                   "Connection: close\r\n"
                                   "\r\n" << std::flush;
        socket->flush_output_device();
#else
        auto post_login_file = evio::create<MyFile>(socket);
        socket->set_source(post_login_file, 1000000, 500000, 1000000);
        post_login_file->open("/home/carlo/projects/aicxx/linuxviewer/linuxviewer/src/POST_login.xml", std::ios_base::in);
#endif
      }
    });

  task->run([this, task](bool success){
      if (!success)
        Dout(dc::warning, "task::ConnectToEndPoint was aborted");
      else
      {
        Dout(dc::notice, "Task with endpoint " << task->get_end_point() << " finished.");
      }
      this->quit();
    });
}

// This is called from the main loop of the GUI during "idle" cycles.
bool LinuxViewerApplication::on_gui_idle()
{
  m_gui_idle_engine.mainloop();

  // Returning true means we want to be called again (more work is to be done).
  return false;
}

void LinuxViewerApplication::append_menu_entries(LinuxViewerMenuBar* menubar)
{
  using namespace menu_keys;

#define ADD(top, entry) \
  menubar->append_menu_entry({top, entry}, std::bind(&LinuxViewerApplication::on_menu_##top##_##entry, this))

  //---------------------------------------------------------------------------
  // Menu buttons that have a call back to this object are added below.
  //
  ADD(File, QUIT);
}

void LinuxViewerApplication::on_menu_File_QUIT()
{
  DoutEntering(dc::notice, "LinuxViewerApplication::on_menu_File_QUIT()");

  // Close all windows and cause the application to return from run(),
  // which will terminate the application (see application.cxx).
  terminate();
}
