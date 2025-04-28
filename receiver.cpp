#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  // Check for correct number of command line arguments
  if (argc != 5) {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  // Extract command line arguments
  std::string server_hostname = argv[1];       // Server address
  int server_port = std::stoi(argv[2]);   // Port number (converted from string)
  std::string username = argv[3];              // Username for login
  std::string room_name = argv[4];             // Room to join

  // Create connection object
  Connection conn;  

  // Connect to the server with the given hostname and port
  conn.connect(server_hostname, server_port);

  // Send rlogin message to identify ourselves to the server
  Message rlogin_msg(TAG_RLOGIN, username);
  if (!conn.send(rlogin_msg)) {
    std::cerr << "Error: failed to send rlogin message.\n";
    return 1;
  }

  // Wait for server response to our login attempt
  Message reply;
  if (!conn.receive(reply)) {
    std::cerr << "Error: failed to receive reply from server.\n";
    return 1;
  }

  // Check if server returned an error
  if (reply.tag == TAG_ERR) {
    std::cerr << reply.data << "\n";
    return 1;
  }

  // Send join message to enter the specified room
  Message join_msg(TAG_JOIN, room_name);
  if (!conn.send(join_msg)) {
    std::cerr << "Error: failed to send join message.\n";
    return 1;
  }

  // Wait for server response to our join request
  if (!conn.receive(reply)) {
    std::cerr << "Error: failed to receive join reply.\n";
    return 1;
  }

  // Check if server returned an error
  if (reply.tag == TAG_ERR) {
    std::cerr << reply.data << "\n";
    return 1;
  }

  // Main message receiving loop (runs continuously to receive chat messages)
  while (true) {
    if (!conn.receive(reply)) {
      // Exit if connection is closed or broken
      break;
    }

    //std::cout << "[DEBUG] Received raw payload: " << reply.data << std::endl;


    // Process delivery messages (actual chat messages)
    if (reply.tag == TAG_DELIVERY) {
      std::string payload = reply.data;

      // Parse the message format: "room:sender:message"
      size_t pos1 = payload.find(':');                // Find first colon (after room)
      size_t pos2 = payload.find(':', pos1 + 1);  // Find second colon (after sender)
      
      // Skip wrongly formatted messages
      if (pos1 == std::string::npos || pos2 == std::string::npos) {
        continue;
      }

      // Extract sender and message from the payload
      std::string sender = payload.substr(pos1 + 1, pos2 - pos1 - 1);
      std::string message = payload.substr(pos2 + 1);

      // Print the message in "sender: message" format
      std::cout << sender << ": " << message << std::endl;
    }
  }

  return 0;
}