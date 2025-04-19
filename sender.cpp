#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  // Check for correct number of command line arguments
  if (argc != 4) {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }

  // Extract command line arguments
  std::string server_hostname = argv[1];       // Server address to connect to
  int server_port = std::stoi(argv[2]);   // Port number (converted from string)
  std::string username = argv[3];             // Username for login

  // Create connection object
  Connection conn;  

  // Connect to the server with the given hostname and port
  conn.connect(server_hostname, server_port);

  // Step 1: Send slogin message to identify as sender
  Message login_msg(TAG_SLOGIN, username);
  if (!conn.send(login_msg)) {
    std::cerr << "Error: failed to send slogin message.\n";
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
    std::cerr << reply.data << std::endl;
    return 1;
  }

  // Step 2: Main command/message input loop
  std::string line;
  while (std::getline(std::cin, line)) {
    // Skip empty lines
    if (line.empty()) continue;

    // Will hold the message we're preparing to send
    Message out_msg;  

    // Check if line starts with '/' (command)
    if (line[0] == '/') {
      std::istringstream iss(line);  // Create stream for parsing command
      std::string cmd;
      iss >> cmd;  // Extract the command

      // Handle different commands
      if (cmd == "/join") {

        // Join a room: /join [room_name]
        std::string room;
        if (iss >> room) {
          out_msg.tag = TAG_JOIN;
          out_msg.data = room;
        } else {
          std::cerr << "Usage: /join [room_name]" << std::endl;
          continue;  // Skip to next input on error
        }
      } else if (cmd == "/leave") {
        // Leave current room
        out_msg.tag = TAG_LEAVE;
        out_msg.data = "";  // No data needed for leave
      } else if (cmd == "/quit") {
        // Quit the program
        out_msg.tag = TAG_QUIT;
        out_msg.data = "bye";  // Payload should be non-empty
        
        // Send quit message and handle response
        conn.send(out_msg);
        if (conn.receive(reply) && reply.tag == TAG_OK) {
          return 0;  // Exit normally if server acknowledges
        } else {
          // Handle error cases
          std::cerr << (reply.tag == TAG_ERR ? reply.data : "Unknown error") << std::endl;
          return 1;
        }
      } else {
        // Unknown command
        std::cerr << "Unknown command: " << cmd << std::endl;
        continue;
      }
    } else {
      // Regular message to send to current room
      out_msg.tag = TAG_SENDALL;
      out_msg.data = line;
    }

    // Send the prepared message to server
    if (!conn.send(out_msg)) {
      std::cerr << "Error: failed to send message.\n";
      return 1;
    }

    // Wait for server response
    if (!conn.receive(reply)) {
      std::cerr << "Error: failed to receive reply from server.\n";
      return 1;
    }

    // Check if server returned an error
    if (reply.tag == TAG_ERR) {
      std::cerr << reply.data << std::endl;
    }
  }

  return 0;
}