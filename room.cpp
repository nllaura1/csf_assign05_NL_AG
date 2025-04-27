#include "guard.h"
#include "message.h"
#include "message_queue.h"
#include "user.h"
#include "room.h"

Room::Room(const std::string &room_name)
  : room_name(room_name) {
  // Initialize the mutex
  pthread_mutex_init(&lock, nullptr);
}

Room::~Room() {
  // Destroy the mutex
  pthread_mutex_destroy(&lock);
  
  // Note: We don't delete User objects here as they're owned by the Server
}

void Room::add_member(User *user) {
  Guard guard(lock); // RAII lock acquisition
  
  // Add user to the members set
  members.insert(user);
}

void Room::remove_member(User *user) {
  Guard guard(lock); // RAII lock acquisition
  
  // Remove user from the members set
  members.erase(user);
}

void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
  Guard guard(lock);
  
  // Create properly formatted message using existing constructor
  std::string payload = room_name + ":" + sender_username + ":" + message_text;
  Message *msg = new Message(TAG_DELIVERY, payload);
  
  // Send to all members
  for (User *user : members) {
    user->mqueue.enqueue(msg);
  }
}