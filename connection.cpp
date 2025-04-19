#include <sstream>
#include <cctype>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "connection.h"


Connection::Connection()
  : m_fd(-1)                // no active connection
  , m_last_result(SUCCESS) { //last operation was successful
}


Connection::Connection(int fd)
  : m_fd(fd)                
  , m_last_result(SUCCESS) { 

  // Initialize the rio_t object for buffered reading
  rio_readinitb(&m_fdbuf, m_fd);
}

// Establish connection to server at specified hostname and port
void Connection::connect(const std::string &hostname, int port) {
  // Convert port to string for open_clientfd function
  std::string port_str = std::to_string(port);
  
  // Open connection to server using CSAPP's helper function
  m_fd = open_clientfd(hostname.c_str(), port_str.c_str());
  
  // Check if connection failed
  if (m_fd < 0) {
    m_last_result = INVALID_MSG;
    return;
  }
  
  // Initialize the rio_t object for buffered reading on new connection
  rio_readinitb(&m_fdbuf, m_fd);
}

// ensures connection is properly closed
Connection::~Connection() {
  // Close the socket if it is currently open
  if (is_open()) {
    Close(m_fd);
  }
}

// Check if connection is currently active
bool Connection::is_open() const {
  return m_fd >= 0; //nonneg
}

// Close the connection if it's open
void Connection::close() {
  if (is_open()) {
    Close(m_fd);  
    m_fd = -1;    
  }
}

// Send a Message object over the connection
bool Connection::send(const Message &msg) {
  // Check if connection is valid
  if (!is_open()) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  // Format the message according to protocol: "tag:data\n"
  std::string formatted_msg = msg.tag + ":" + msg.data + "\n";

  // Send the complete message using reliable write
  ssize_t bytes_sent = rio_writen(m_fd, formatted_msg.c_str(), formatted_msg.size());
  
  // Verify entire message was sent
  if (bytes_sent != (ssize_t)formatted_msg.size()) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  m_last_result = SUCCESS;
  return true;
}

// Receive message from the connection
bool Connection::receive(Message &msg) {
  // Check if connection is valid
  if (!is_open()) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  // Buffer for reading the message (plus null terminator)
  char buf[Message::MAX_LEN + 1];
  
  // Read a line using buffered I/O
  ssize_t bytes_read = rio_readlineb(&m_fdbuf, buf, sizeof(buf));
  
  // Check for read errors or EOF
  if (bytes_read <= 0) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  // Remove trailing newline if present
  if (buf[bytes_read - 1] == '\n') {
    buf[bytes_read - 1] = '\0';
    bytes_read--;
  }

  // Convert buffer to string for parsing
  std::string received_msg(buf);
  
  // Find the colon separator between tag and data
  size_t colon_pos = received_msg.find(':');
  
  // Validate message format
  if (colon_pos == std::string::npos) {
    m_last_result = INVALID_MSG;
    return false;
  }

  // Extract tag (before colon) and data (after colon)
  msg.tag = received_msg.substr(0, colon_pos);
  msg.data = received_msg.substr(colon_pos + 1);

  m_last_result = SUCCESS;
  return true;
}