#include "sys.h"
#include "LinuxViewerApplication.h"
#include "LinuxViewerMenuBar.h"
#include "statefultask/AIEngine.h"
#include "socket-task/ConnectToEndPoint.h"
#include "evio/Socket.h"
#include "evio/protocol/http.h"
#include <functional>

namespace http = evio::protocol::http;

//static
std::unique_ptr<LinuxViewerApplication> LinuxViewerApplication::create(AIEngine& gui_idle_engine)
{
  return std::make_unique<LinuxViewerApplication>(gui_idle_engine);
}

class XMLDecoder : public evio::protocol::Decoder
{
 private:
  void decode(int& allow_deletion_count, evio::MsgBlock&& msg) override
  {
    Dout(dc::notice, "Received: \"" << msg << "\"");
  }
};

class MySocket : public evio::Socket
{
 private:
  http::ResponseHeadersDecoder m_input_decoder;
  XMLDecoder m_xml_decoder;
  evio::OutputStream m_output_stream;

 public:
  MySocket() : m_input_decoder({{"application/xml", m_xml_decoder}})
  {
    set_protocol_decoder(m_input_decoder);
    set_source(m_output_stream);
  }

  evio::OutputStream& output_stream() { return m_output_stream; }
};

// Called when the main instance (as determined by the GUI) of the application is starting.
void LinuxViewerApplication::on_main_instance_startup()
{
  // Run a test task.
  boost::intrusive_ptr<task::ConnectToEndPoint> task = new task::ConnectToEndPoint(CWDEBUG_ONLY(true));
  auto socket = evio::create<MySocket>();
  task->set_socket(socket);
  task->set_end_point(AIEndPoint("misfitzgrid.com", 8002));

  task->run([this, task](bool success){
      if (!success)
        Dout(dc::warning, "task::ConnectToEndPoint was aborted");
      else
      {
        Dout(dc::notice, "Task with endpoint " << task->get_end_point() << " finished.");
      }
      this->quit();
    });
  // Must do a flush or else the buffer won't be written to the socket at all; this flush
  // does not block though, it only starts watching the fd for readability and then writes
  // the buffer to the fd when possible.
  // If the socket was closed in the meantime because it permanently failed to connect
  // or connected but then the connection was terminated for whatever reason; then the
  // flush will print a debug output (WARNING: The device is not writable!) and the contents
  // of the buffer are discarded.
  socket->output_stream() << "GET /get_grid_info HTTP/1.1\r\n"
                             "Host: misfitzgrid.com:8002\r\n"
                             "Accept-Encoding: deflate, gzip\r\n"
                             "Connection: keep-alive\r\n"
                             "Keep-alive: 300\r\n"
                             "Accept: application/llsd+xml\r\n"
                             "Content-Type: application/llsd+xml\r\n"
                             "X-SecondLife-UDP-Listen-Port: 46055\r\n"
                             "\r\n" << std::flush;
  socket->flush_output_device();
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
