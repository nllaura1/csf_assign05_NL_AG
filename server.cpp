#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

struct ClientInfo {
    Connection* conn;
    Server* server;
    MessageQueue* mqueue;
    Room* room;
    User* user;  
};

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace {
    void chat_with_sender(ClientInfo* client) {
        Connection* conn = client->conn;
        Server* server = client->server;
    
        while (true) {
            Message msg;
            if (!conn->receive(msg)) {
                break; // client disconnected
            }
    
            if (msg.tag == TAG_SENDALL) {
                if (client->room) {
                    client->room->broadcast_message(client->user->username, msg.data);
                    conn->send(Message(TAG_OK, "message sent"));
                } else {
                    conn->send(Message(TAG_ERR, "not in a room"));
                }
            } else if (msg.tag == TAG_JOIN) {
                Room* new_room = server->find_or_create_room(msg.data);
                if (client->room) {
                    client->room->remove_member(client->user);
                }
                client->room = new_room;
                client->room->add_member(client->user, client->mqueue);
                conn->send(Message(TAG_OK, "joined room " + client->room->get_room_name()));
            } else if (msg.tag == TAG_LEAVE) {
                if (client->room) {
                    client->room->remove_member(client->user);
                    conn->send(Message(TAG_OK, "left room " + client->room->get_room_name()));
                    client->room = nullptr;
                } else {
                    conn->send(Message(TAG_ERR, "not in a room"));
                }
            } else if (msg.tag == TAG_QUIT) {
                conn->send(Message(TAG_OK, "bye!"));
                break; // Exit the sender loop
            } else {
                conn->send(Message(TAG_ERR, "invalid command"));
            }
        }
    }
    

void chat_with_receiver(ClientInfo* client) {
  Connection* conn = client->conn;

  // Step 1: Receiver must first send JOIN
  Message join_msg;
  if (!conn->receive(join_msg)) {
      conn->send(Message(TAG_ERR, "invalid message"));
      return;
  }

  if (join_msg.tag != TAG_JOIN) {
      conn->send(Message(TAG_ERR, "Expected JOIN"));
      return;
  }

  // Step 2: Add receiver to the room
  Room* new_room = client->server->find_or_create_room(join_msg.data);
  if (client->room) {
      client->room->remove_member(client->user);
  }
  client->room = new_room;
  client->room->add_member(client->user, client->mqueue);
  conn->send(Message(TAG_OK, "welcome"));

  // Step 3: Receiver now just dequeues and sends messages
  while (true) {
      Message* msg = client->mqueue->dequeue();
      if (msg) {
          if (!conn->send(*msg)) {
              delete msg;
              break; // disconnected
          }
          delete msg;
      }
  }
}



void *worker(void *arg) {
    pthread_detach(pthread_self());

    ClientInfo* client = static_cast<ClientInfo*>(arg);

    Connection* conn = client->conn;

    Message login_msg;
    if (!conn->receive(login_msg)) {
        goto cleanup;
    }

    if (login_msg.tag != TAG_SLOGIN && login_msg.tag != TAG_RLOGIN) {
        conn->send(Message(TAG_ERR, "expected slogin or rlogin"));
        goto cleanup;
    }

    if (login_msg.data.empty()) {
        conn->send(Message(TAG_ERR, "empty username"));
        goto cleanup;
    }

    client->user = new User(login_msg.data);
    client->mqueue = new MessageQueue();
    client->room = nullptr;

    if (login_msg.tag == TAG_SLOGIN) {
        conn->send(Message(TAG_OK, "logged in as " + login_msg.data));
        chat_with_sender(client);
    } else if (login_msg.tag == TAG_RLOGIN) {
        conn->send(Message(TAG_OK, "logged in as " + login_msg.data));
        chat_with_receiver(client);
    }

cleanup:
    if (client->room) {
        client->room->remove_member(client->user);
    }
    delete client->user;
    delete client->mqueue;
    delete client->conn;
    delete client;
    return nullptr;
}

}

////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
  : m_port(port)
  , m_ssock(-1) {
    pthread_mutex_init(&m_lock, nullptr);
}

Server::~Server() {
    pthread_mutex_destroy(&m_lock);
}

bool Server::listen() {
    m_ssock = open_listenfd(std::to_string(m_port).c_str());
    return (m_ssock >= 0);
}

void Server::handle_client_requests() {
    while (true) {
        int csock = Accept(m_ssock, nullptr, nullptr);
        if (csock < 0) {
            continue;
        }
        Connection* conn = new Connection(csock);
        ClientInfo* info = new ClientInfo{conn, this, nullptr, nullptr, nullptr};
        pthread_t thr_id;
        pthread_create(&thr_id, nullptr, worker, info);
    }
}

Room *Server::find_or_create_room(const std::string &room_name) {
    Guard guard(m_lock);
    Room* &room = m_rooms[room_name];
    if (!room) {
        room = new Room(room_name);
    }
    return room;
}
