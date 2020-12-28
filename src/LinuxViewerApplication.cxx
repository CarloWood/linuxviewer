#include "sys.h"
#include "LinuxViewerApplication.h"
#include "LinuxViewerMenuBar.h"
#include "statefultask/AIEngine.h"
#include "helloworld-task/HelloWorld.h"
#include "socket-task/ConnectToEndPoint.h"
#include "evio/Socket.h"
#include <functional>
#include <libcwd/buf2str.h>

//static
std::unique_ptr<LinuxViewerApplication> LinuxViewerApplication::create(AIEngine& gui_idle_engine)
{
  return std::make_unique<LinuxViewerApplication>(gui_idle_engine);
}

// Accept input of the form:
//
// HTTP/1.1 200 OK
// Some-Header: its, value=here
// ...
//
// Currently closes input on an empty line.
class HttpHeadersDecoder : public evio::protocol::Decoder
{
 public:
  // The highest protocol that we understand.
  static constexpr int s_http_major = 1;
  static constexpr int s_http_minor = 1;
  // We only understand major version 1.
  int m_http_minor;     // 0 or 1.
  bool m_status_line_received;
  bool m_protocol_error;
  int m_status_code;
  std::string m_reason_phrase;

  // Until the server tells us otherwise, we have to speak verion 1.0.
  // m_http_minor is updated at the moment that m_status_line_received
  // is set.
  HttpHeadersDecoder() : m_http_minor(0), m_status_line_received(false), m_protocol_error(false), m_status_code(0) { }

 public:
  // Most HTML header lines are very short (in the order of 32 bytes or less),
  // but some header lines, most notably cookies can be rather long (say, 300 bytes).
  // Specifying values of up to 512 bytes have no negative impact, so lets use
  // something on the larger side (so that > 99.9% of the headers will fit
  // in the allocated buffer that will be 6 times as large, or 1536 bytes.
  size_t average_message_length() const override { return 256; }

 protected:
   void decode(int& allow_deletion_count, evio::MsgBlock&& msg) override;
};

void HttpHeadersDecoder::decode(int& CWDEBUG_ONLY(allow_deletion_count), evio::MsgBlock&& msg)
{
  // Just print what was received.
  DoutEntering(dc::notice, "HttpHeadersDecoder::decode({" << allow_deletion_count << "}, \"" << libcwd::buf2str(msg.get_start(), msg.get_size()) << "\") [" << this << ']');

  if (!m_status_line_received)
  {
    // This is obviously true for the first line.
    // If the first line doesn't match the required responds then that is an error,
    // otherwise we have the status line.
    // The required form is (RFC7230):
    //
    // status-line  = HTTP-version SP status-code SP reason-phrase CRLF
    // HTTP-version = HTTP-name "/" DIGIT "." DIGIT
    // HTTP-name    = %x48.54.54.50 ; "HTTP", case-sensitive
    // status-code  = 3DIGIT
    // reason-phrase  = *( HTAB / SP / VCHAR / obs-text )
    // obs-text       = %x80-FF
    //
    // Where (RFC5234):
    // DIGIT          = %x30-39 ; 0-9
    // HTAB           = %x09 ; horizontal tab
    // SP             = %x20
    // VCHAR          = %x21-7E ; visible (printing) characters
    // CRLF           =  CR LF ; Internet standard newline
    // CR             =  %x0D ; carriage return
    // LF             =  %x0A ; linefeed

    // Hence the string must look like:
    //
    //     HTTP/1.1 200 <reason-phrase>\r\n
    //
    // and thus have a length of at least 15 characters.
    //
    char const* ptr = msg.get_start();
    if (msg.get_size() < 15)
      m_protocol_error = true;
    else if (strncmp(ptr, "HTTP/1.", 7) != 0)
      m_protocol_error = true;
    else
    {
      ptr += 7;
      if (*ptr != '0' && *ptr != '1' || ptr[1] != ' ')
        m_protocol_error = true;
      else
      {
        m_http_minor = *ptr - '0';
        ptr += 2;
        for (int digitn = 0; digitn < 3; ++digitn)
        {
          int digit = *ptr++ - '0';
          if (digit < 0 || digit > 9)
          {
            m_protocol_error = true;
            break;
          }
          m_status_code = 10 * m_status_code + digit;
        }
        if (*ptr++ != ' ')
          m_protocol_error = true;
        else
        {
          int reason_phrase_length = msg.get_size() - (ptr - msg.get_start()) - 2;
          for (int reason_phrase_charn = 0; reason_phrase_charn < reason_phrase_length; ++reason_phrase_charn)
          {
            // reason_phrase_char.
            unsigned char rpc = *ptr++;
            if (rpc != 0x09 && rpc != 0x20 && !(0x21 <= rpc && rpc <= 0x7e) && !(0x80 <= rpc /* && rpc <= 0xff */))
            {
              m_protocol_error = true;
              break;
            }
            m_reason_phrase += rpc;
          }
          if (*ptr != '\r' || ptr[1] != '\n')
            m_protocol_error = true;
        }
      }
    }
    if (!m_protocol_error)
      m_status_line_received = true;
  }

  if (m_protocol_error || m_status_code != 200 ||
      (msg.get_size() == 2 && strncmp(msg.get_start(), "\r\n", 2) == 0)) // Stop when we see an empty line.
  {
    if (m_protocol_error)
      Dout(dc::warning, "Received an undecodable message");
    else if (m_status_code != 200)
      Dout(dc::warning, "Received status code " << m_status_code << " " << m_reason_phrase);
    stop_input_device();
  }
}

class MySocket : public evio::Socket
{
 private:
  HttpHeadersDecoder m_input_decoder;
  evio::OutputStream m_output_stream;

 public:
  MySocket()
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
  socket->close();
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
