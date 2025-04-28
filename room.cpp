#include "guard.h"
#include "message.h"
#include "message_queue.h"
#include "user.h"
#include "room.h"

// Room constructor
// Initializes a new chat room with the given name
Room::Room(const std::string &room_name)
 // Initialize the room name
  : room_name(room_name) { 
    // Initialize the mutex for thread safety
    pthread_mutex_init(&lock, nullptr);  
}

// Room destructor
// Cleans up resources when the room is destroyed
Room::~Room() {
    pthread_mutex_destroy(&lock);  // Destroy the mutex
}

// Add a member to the room
// Associates a user with their message queue for receiving messages
void Room::add_member(User *user, MessageQueue *mqueue) {
    Guard guard(lock);  // Lock the mutex 
    members[user] = mqueue;  // Add the user and their message queue to the members map
}

// Remove a member from the room
// Disassociates a user from the room
void Room::remove_member(User *user) {
    Guard guard(lock);  // Lock the mutex
    members.erase(user);  // Remove the user from the members map
}

// Broadcast a message to all room members
// Sends a message from one user to all other users in the room
void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
    Guard guard(lock);  // Lock the mutex to safely access members

    // Format the message payload as "roomname:sender:message"
    std::string payload = room_name + ":" + sender_username + ":" + message_text;
    
    // Log the broadcast for debugging/monitoring
    printf("[server] Broadcasting from %s: %s\n", sender_username.c_str(), message_text.c_str());

    // Iterate through all members in the room
    for (auto &entry : members) {
        MessageQueue* mqueue = entry.second;  // Get the member's message queue
        
        // Create a new message with the formatted payload
        Message* msg = new Message(TAG_DELIVERY, payload);
        
        // Add the message to the member's queue
        mqueue->enqueue(msg);
        
        // Log the enqueue operation for debugging
        printf("[queue] Enqueued message: %s\n", payload.c_str());
    }
}