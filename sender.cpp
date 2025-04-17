#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];

  Connection conn;

  conn.connect(server_hostname, server_port);

  // Step 1: send slogin
  Message login_msg(TAG_SLOGIN, username);
  if (!conn.send(login_msg)) {
    std::cerr << "Error: failed to send slogin message.\n";
    return 1;
  }

  Message reply;
  if (!conn.receive(reply)) {
    std::cerr << "Error: failed to receive reply from server.\n";
    return 1;
  }

  if (reply.tag == TAG_ERR) {
    std::cerr << reply.data << std::endl;
    return 1;
  }

  // Step 2: command/message loop
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;

    Message out_msg;

    if (line[0] == '/') {
      std::istringstream iss(line);
      std::string cmd;
      iss >> cmd;

      if (cmd == "/join") {
        std::string room;
        if (iss >> room) {
          out_msg.tag = TAG_JOIN;
          out_msg.data = room;
        } else {
          std::cerr << "Usage: /join [room_name]" << std::endl;
          continue;
        }
      } else if (cmd == "/leave") {
        out_msg.tag = TAG_LEAVE;
        out_msg.data = "";
      } else if (cmd == "/quit") {
        out_msg.tag = TAG_QUIT;
        out_msg.data = "bye"; // payload can be arbitrary but must be non-empty
        conn.send(out_msg);
        if (conn.receive(reply) && reply.tag == TAG_OK) {
          return 0;
        } else {
          std::cerr << (reply.tag == TAG_ERR ? reply.data : "Unknown error") << std::endl;
          return 1;
        }
      } else {
        std::cerr << "Unknown command: " << cmd << std::endl;
        continue;
      }
    } else {
      // Send as message
      out_msg.tag = TAG_SENDALL;
      out_msg.data = line;
    }

    // Send the message and wait for response
    if (!conn.send(out_msg)) {
      std::cerr << "Error: failed to send message.\n";
      return 1;
    }

    if (!conn.receive(reply)) {
      std::cerr << "Error: failed to receive reply from server.\n";
      return 1;
    }

    if (reply.tag == TAG_ERR) {
      std::cerr << reply.data << std::endl;
    }
  }

  return 0;
}
