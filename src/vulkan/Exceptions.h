#pragma once

#include "queues/QueueFlags.h"
#include <exception>
#include <sstream>

namespace vulkan {

class ExceptionStringMessageTransfer : public std::ostringstream
{
 private:
  std::string& m_message;

 public:
  ExceptionStringMessageTransfer(std::string& message) : m_message(message) { }
  ~ExceptionStringMessageTransfer() { m_message = str(); }
};

class ExceptionString : public std::exception
{
 private:
  std::string m_message;

 protected:
  ExceptionStringMessageTransfer message() { return m_message; }

 public:
  char const* what() const noexcept override final
  {
    return m_message.c_str();
  }
};

// Thrown when Device::acquireNextImageKHR returns Result::eErrorOutOfDateKHR.
struct OutOfDateKHR_Exception : public std::exception
{
  char const* what() const noexcept override final
  {
    return "eErrorOutOfDateKHR";
  }
};

struct OutOfQueues_Exception : public ExceptionString
{
  vulkan::QueueFlags m_queue_flags;
  uint32_t m_number_of_queues;

  OutOfQueues_Exception(vulkan::QueueFlags queue_flags, uint32_t number_of_queues) :
    m_queue_flags(queue_flags), m_number_of_queues(number_of_queues)
  {
    message() << "Trying to acquire a queue with flags " << m_queue_flags << ", but we have run out; there were only " << m_number_of_queues << " such queues.";
  }
};

} // namespace vulkan
