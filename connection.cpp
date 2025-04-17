#include <sstream>
#include <cctype>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "connection.h"

Connection::Connection()
  : m_fd(-1)
  , m_last_result(SUCCESS) {
}

Connection::Connection(int fd)
  : m_fd(fd)
  , m_last_result(SUCCESS) {
  // Initialize the rio_t object for reading
  rio_readinitb(&m_fdbuf, m_fd);
}

void Connection::connect(const std::string &hostname, int port) {
  // Convert port to string for open_clientfd
  std::string port_str = std::to_string(port);
  
  // Open connection to server
  m_fd = open_clientfd(hostname.c_str(), port_str.c_str());
  if (m_fd < 0) {
    m_last_result = INVALID_MSG;
    return;
  }
  
  // Initialize the rio_t object for reading
  rio_readinitb(&m_fdbuf, m_fd);
}

Connection::~Connection() {
  // Close the socket if it is open
  if (is_open()) {
    Close(m_fd);
  }
}

bool Connection::is_open() const {
  return m_fd >= 0;
}

void Connection::close() {
  if (is_open()) {
    Close(m_fd);
    m_fd = -1;
  }
}

bool Connection::send(const Message &msg) {
  if (!is_open()) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  // Format the message according to protocol: "tag:data\n"
  std::string formatted_msg = msg.tag + ":" + msg.data + "\n";

  // Send the message
  ssize_t bytes_sent = rio_writen(m_fd, formatted_msg.c_str(), formatted_msg.size());
  
  if (bytes_sent != (ssize_t)formatted_msg.size()) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  m_last_result = SUCCESS;
  return true;
}

bool Connection::receive(Message &msg) {
  if (!is_open()) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  char buf[Message::MAX_LEN + 1];
  ssize_t bytes_read = rio_readlineb(&m_fdbuf, buf, sizeof(buf));
  
  if (bytes_read <= 0) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  // Remove trailing newline if present
  if (buf[bytes_read - 1] == '\n') {
    buf[bytes_read - 1] = '\0';
    bytes_read--;
  }

  // Parse the message into tag and data
  std::string received_msg(buf);
  size_t colon_pos = received_msg.find(':');
  
  if (colon_pos == std::string::npos) {
    m_last_result = INVALID_MSG;
    return false;
  }

  msg.tag = received_msg.substr(0, colon_pos);
  msg.data = received_msg.substr(colon_pos + 1);

  m_last_result = SUCCESS;
  return true;
}