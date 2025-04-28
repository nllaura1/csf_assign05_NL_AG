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

// Structure to hold information about each connected client
struct ClientInfo {
    Connection* conn;    // Network connection to the client
    Server* server;      // Reference to the main server
    MessageQueue* mqueue; // Message queue for receiving messages
    Room* room;          // Current room the client is in
    User* user;          // User information
};

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace {
    // Function to handle communication with a sender client
    void chat_with_sender(ClientInfo* client) {
        Connection* conn = client->conn;
        Server* server = client->server;
    
        while (true) {
            Message msg;
            // Receive a message from the client
            if (!conn->receive(msg)) {
                break; // client disconnected
            }
    
            // Handle different message types from sender
            if (msg.tag == TAG_SENDALL) {
                // Broadcast message to all in the room
                if (client->room) {
                    client->room->broadcast_message(client->user->username, msg.data);
                    conn->send(Message(TAG_OK, "message sent"));
                } else {
                    conn->send(Message(TAG_ERR, "not in a room"));
                }
            } else if (msg.tag == TAG_JOIN) {
                // Join a room (or create if new)
                Room* new_room = server->find_or_create_room(msg.data);
                if (client->room) {
                    client->room->remove_member(client->user);
                }
                client->room = new_room;
                client->room->add_member(client->user, client->mqueue);
                conn->send(Message(TAG_OK, "joined room " + client->room->get_room_name()));
            } else if (msg.tag == TAG_LEAVE) {
                // Leave current room
                if (client->room) {
                    client->room->remove_member(client->user);
                    conn->send(Message(TAG_OK, "left room " + client->room->get_room_name()));
                    client->room = nullptr;
                } else {
                    conn->send(Message(TAG_ERR, "not in a room"));
                }
            } else if (msg.tag == TAG_QUIT) {
                // Quit the connection
                conn->send(Message(TAG_OK, "bye!"));
                break; // Exit the sender loop
            } else {
                conn->send(Message(TAG_ERR, "invalid command"));
            }
        }
    }
    
    // Function to handle communication with a receiver client
    void chat_with_receiver(ClientInfo* client) {
        Connection* conn = client->conn;

        // Step 1: Receiver must first send JOIN to specify which room to receive from
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

        // Step 3: Receiver continuously dequeues and sends messages from the room
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

    // Worker thread function that handles each client connection
    void *worker(void *arg) {
        pthread_detach(pthread_self()); // Detach thread so it cleans up automatically

        ClientInfo* client = static_cast<ClientInfo*>(arg);
        Connection* conn = client->conn;

        // Step 1: Receive login message
        Message login_msg;
        if (!conn->receive(login_msg)) {
            goto cleanup; // connection error
        }

        // Validate login message
        if (login_msg.tag != TAG_SLOGIN && login_msg.tag != TAG_RLOGIN) {
            conn->send(Message(TAG_ERR, "expected slogin or rlogin"));
            goto cleanup;
        }

        if (login_msg.data.empty()) {
            conn->send(Message(TAG_ERR, "empty username"));
            goto cleanup;
        }

        // Set up client information
        client->user = new User(login_msg.data);
        client->mqueue = new MessageQueue();
        client->room = nullptr;

        // Handle sender or receiver based on login type
        if (login_msg.tag == TAG_SLOGIN) {
            conn->send(Message(TAG_OK, "logged in as " + login_msg.data));
            chat_with_sender(client); // Enter sender loop
        } else if (login_msg.tag == TAG_RLOGIN) {
            conn->send(Message(TAG_OK, "logged in as " + login_msg.data));
            chat_with_receiver(client); // Enter receiver loop
        }

    cleanup:
        // Clean up resources when client disconnects
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

// Server constructor
Server::Server(int port)
  : m_port(port)      // Set server port
  , m_ssock(-1) {     // Initialize socket to invalid
    pthread_mutex_init(&m_lock, nullptr); // Initialize mutex for thread safety
}

// Server destructor
Server::~Server() {
    pthread_mutex_destroy(&m_lock); // Clean up mutex
}

// Start listening on the server port
bool Server::listen() {
    m_ssock = open_listenfd(std::to_string(m_port).c_str());
    return (m_ssock >= 0); // Return true if listening socket was created successfully
}

// Main server loop to handle incoming client connections
void Server::handle_client_requests() {
    while (true) {
        // Accept new client connection
        int csock = Accept(m_ssock, nullptr, nullptr);
        if (csock < 0) {
            continue; // Skip if accept failed
        }
        // Create new connection and client info
        Connection* conn = new Connection(csock);
        ClientInfo* info = new ClientInfo{conn, this, nullptr, nullptr, nullptr};
        // Create worker thread to handle this client
        pthread_t thr_id;
        pthread_create(&thr_id, nullptr, worker, info);
    }
}

// Find or create a room with the given name
Room *Server::find_or_create_room(const std::string &room_name) {
    Guard guard(m_lock); // Thread-safe access to rooms map
    Room* &room = m_rooms[room_name]; // Get reference to room pointer
    if (!room) {
        room = new Room(room_name); // Create new room if it doesn't exist
    }
    return room;
}