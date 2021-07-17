#include "sys.h"
#include "LinuxViewerApplication.h"
#include "LinuxViewerMenuBar.h"
#include "protocols/xmlrpc/response/LoginResponse.h"
#include "protocols/xmlrpc/request/LoginToSimulator.h"
#include "protocols/GridInfoDecoder.h"
#include "protocols/GridInfo.h"
#include "evio/protocol/xmlrpc/Encoder.h"
#include "evio/protocol/xmlrpc/Decoder.h"
#include "statefultask/AIEngine.h"
#include "xmlrpc-task/XML_RPC_MethodCall.h"
#include "evio/Socket.h"
#include "evio/File.h"
#include "evio/protocol/http.h"
#include "evio/protocol/EOFDecoder.h"
#include "utils/debug_ostream_operators.h"
#include "utils/threading/Gate.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <functional>
#include <regex>
#include <iterator>

namespace utils { using namespace threading; }
namespace http = evio::protocol::http;

class MySocket : public evio::Socket
{
 private:
  http::ResponseHeadersDecoder m_input_decoder;
  GridInfoDecoder m_grid_info_decoder;
  GridInfo m_grid_info;
  evio::protocol::xmlrpc::Decoder m_xml_rpc_decoder;
  xmlrpc::LoginResponse m_login_response;
  evio::OutputStream m_output_stream;

 public:
  MySocket() :
    m_input_decoder(
        {{"application/xml", m_grid_info_decoder},
         {"text/xml", m_xml_rpc_decoder}}
        ),
    m_grid_info_decoder(m_grid_info),
    m_xml_rpc_decoder(m_login_response)
  {
    set_source(m_output_stream);
    set_protocol_decoder(m_input_decoder);
  }

  evio::OutputStream& output_stream() { return m_output_stream; }
};

class MyTestFile : public evio::File
{
 private:
  LinuxViewerApplication* m_application;
  evio::protocol::xmlrpc::Decoder m_xml_rpc_decoder;
  xmlrpc::LoginResponse m_login_response;

 public:
  MyTestFile(LinuxViewerApplication* application) : m_application(application), m_xml_rpc_decoder(m_login_response)
  {
    set_protocol_decoder(m_xml_rpc_decoder);
  }

  void closed(int& CWDEBUG_ONLY(allow_deletion_count)) override
  {
    DoutEntering(dc::notice, "MyTestFile::closed({" << allow_deletion_count << "})");
    m_application->main_quit();
  }
};

class MyInputFile : public evio::File
{
 private:
  boost::intrusive_ptr<MySocket> m_linked_output_device;

 public:
  MyInputFile(boost::intrusive_ptr<MySocket> linked_output_device) : m_linked_output_device(std::move(linked_output_device)) { }

  // If the file is closed we wrote everything, so call flush on the socket.
  void closed(int& UNUSED_ARG(allow_deletion_count)) override { m_linked_output_device->flush_output_device(); }
};

class MyOutputFile : public evio::File
{
};

// Called when the main instance (as determined by the GUI) of the application is starting.
void LinuxViewerApplication::on_main_instance_startup()
{
  DoutEntering(dc::notice, "LinuxViewerApplication::on_main_instance_startup()");

  // Allow the main thread to wait until the test finished.
  utils::Gate test_finished;

#if 0
  // This performs a login to MG, using pre-formatted data from a file.
  xmlrpc::LoginToSimulatorCreate login_to_simulator;
  std::ifstream ifs("/home/carlo/projects/aicxx/linuxviewer/POST_login_boost.xml");
  boost::archive::text_iarchive ia(ifs);
  ia >> login_to_simulator;

  auto login_request = std::make_shared<xmlrpc::LoginToSimulator>(login_to_simulator);
  auto login_response = std::make_shared<xmlrpc::LoginResponse>();

  AIEndPoint end_point("misfitzgrid.com", 8002);

  boost::intrusive_ptr<task::XML_RPC_MethodCall> task = new task::XML_RPC_MethodCall(CWDEBUG_ONLY(true));
  task->set_end_point(std::move(end_point));
  task->set_request_object(login_request);
  task->set_response_object(login_response);
#endif

#if 0
  std::stringstream ss;
  evio::protocol::xmlrpc::Encoder encoder(ss);

  try
  {
    encoder << lts;
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
  }

  {
    std::regex hash_re("[0-9a-f]{32}");
    LibcwDoutScopeBegin(LIBCWD_DEBUGCHANNELS, libcwd::libcw_do, dc::notice)
    LibcwDoutStream << "Encoding: ";
    std::string const& text = ss.str();
    std::regex_replace(std::ostreambuf_iterator<char>(LibcwDoutStream), text.begin(), text.end(), hash_re, "********************************");
    LibcwDoutScopeEnd;
  }
#endif

#if 0
  // Run a test task.
  boost::intrusive_ptr<task::ConnectToEndPoint> task = new task::ConnectToEndPoint(CWDEBUG_ONLY(true));
  auto socket = evio::create<MySocket>();
  task->set_socket(socket);
  task->set_end_point(std::move(end_point));
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
        auto post_login_file = evio::create<MyInputFile>(socket);
        socket->set_source(post_login_file, 1000000, 500000, 1000000);
        post_login_file->open("/home/carlo/projects/aicxx/linuxviewer/linuxviewer/src/POST_login.xml", std::ios_base::in);
#endif
      }
    });
#else
//  auto input_file = evio::create<MyTestFile>(this);
//  input_file->open("/home/carlo/projects/aicxx/linuxviewer/linuxviewer/src/login_Response_formatted.xml", std::ios_base::in);
#endif

#if 0
  auto output_file = evio::create<MyOutputFile>();
  output_file->set_source(socket, 4096, 3000, std::numeric_limits<size_t>::max());
  output_file->open("/home/carlo/projects/aicxx/linuxviewer/linuxviewer/src/login_Response.xml", std::ios_base::out);
#endif

#if 0
  // This run the task that was created above.
  task->run([this, task, &test_finished](bool success){
      if (!success)
        Dout(dc::warning, "task::XML_RPC_MethodCall was aborted");
      else
        Dout(dc::notice, "Task with endpoint " << task->get_end_point() << " finished; response: " << *static_cast<xmlrpc::LoginResponse*>(task->get_response_object().get()));
      this->quit();
      test_finished.open();
    });

  test_finished.wait();
#endif
}

void LinuxViewerApplication::append_menu_entries(LinuxViewerMenuBar* menubar)
{
  using namespace menu_keys;

#define ADD(top, entry) \
  menubar->append_menu_entry({top, entry}, std::bind(&LinuxViewerApplication::on_menu_##top##_##entry, this))

  //---------------------------------------------------------------------------
  // Menu buttons that have a call back to this object are added below.
  //

  //FIXME: not compiling with glfw3
  //ADD(File, QUIT);
}

void LinuxViewerApplication::on_menu_File_QUIT()
{
  DoutEntering(dc::notice, "LinuxViewerApplication::on_menu_File_QUIT()");

  // Close all windows and cause the application to return from run(),
  // which will terminate the application (see application.cxx).
  terminate();
}

int main(int argc, char* argv[])
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  ApplicationCreateInfo application_create_info = {
    .application_name = "LinuxViewerApplication"
  };

  WindowCreateInfo main_window_create_info = {
    // gui::WindowCreateInfo
    {
      // glfw::WindowHints
      { .focused = false,
        .centerCursor = false,
        .clientApi = glfw::ClientApi::None },
      // gui::WindowCreateInfoExt
      { .width = 500,
        .height = 800,
        .title = "Main window title" }
    }
  };

  // Create main application.
  LinuxViewerApplication application(application_create_info);
  try
  {
    // Run main application.
    application.run(argc, argv, main_window_create_info);

    // Application terminated cleanly.
    application.join_event_loop();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error << " caught in LinuxViewerApplication.cxx");
  }

  Dout(dc::notice, "Leaving main()");
}
