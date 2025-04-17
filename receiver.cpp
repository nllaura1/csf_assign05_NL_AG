#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string room_name = argv[4];

  Connection conn;

  if (!conn.connect(server_hostname, server_port)) {
    std::cerr << "Error: could not connect to server.\n";
    return 1;
  }

  // Send rlogin message
  Message rlogin_msg(TAG_RLOGIN, username);
  if (!conn.send(rlogin_msg)) {
    std::cerr << "Error: failed to send rlogin message.\n";
    return 1;
  }

  Message reply;
  if (!conn.receive(reply)) {
    std::cerr << "Error: failed to receive reply from server.\n";
    return 1;
  }

  if (reply.tag == TAG_ERR) {
    std::cerr << reply.data << "\n";
    return 1;
  }

  // Send join message
  Message join_msg(TAG_JOIN, room_name);
  if (!conn.send(join_msg)) {
    std::cerr << "Error: failed to send join message.\n";
    return 1;
  }

  if (!conn.receive(reply)) {
    std::cerr << "Error: failed to receive join reply.\n";
    return 1;
  }

  if (reply.tag == TAG_ERR) {
    std::cerr << reply.data << "\n";
    return 1;
  }

  // Main loop to receive messages
  while (true) {
    if (!conn.receive(reply)) {
      // Exit if connection closed or broken
      break;
    }

    if (reply.tag == TAG_DELIVERY) {
      std::string payload = reply.data;

      // Parse: room:sender:message
      size_t pos1 = payload.find(':');
      size_t pos2 = payload.find(':', pos1 + 1);
      if (pos1 == std::string::npos || pos2 == std::string::npos) {
        continue; // malformed message
      }

      std::string sender = payload.substr(pos1 + 1, pos2 - pos1 - 1);
      std::string message = payload.substr(pos2 + 1);

      std::cout << sender << ": " << message << std::endl;
    }
  }

  return 0;
}
