#include "guard.h"
#include "message.h"
#include "message_queue.h"
#include "user.h"
#include "room.h"

Room::Room(const std::string &room_name)
  : room_name(room_name) {
    pthread_mutex_init(&lock, nullptr);
}

Room::~Room() {
    pthread_mutex_destroy(&lock);
}

void Room::add_member(User *user, MessageQueue *mqueue) {
    Guard guard(lock);
    members[user] = mqueue;
}

void Room::remove_member(User *user) {
    Guard guard(lock);
    members.erase(user);
}

void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
    Guard guard(lock);

    std::string payload = room_name + ":" + sender_username + ":" + message_text;
    //printf("[server] Broadcasting from %s: %s\n", sender_username.c_str(), message_text.c_str());

    for (auto &entry : members) {
        MessageQueue* mqueue = entry.second;
        Message* msg = new Message(TAG_DELIVERY, payload);
        mqueue->enqueue(msg);
    }
}

