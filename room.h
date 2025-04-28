#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <map>
#include <pthread.h>
#include "user.h"
#include "message_queue.h"

class Room {
public:
    Room(const std::string &room_name);
    ~Room();

    void add_member(User *user, MessageQueue *mqueue);
    void remove_member(User *user);
    void broadcast_message(const std::string &sender_username, const std::string &message_text);

private:
    std::string room_name;
    pthread_mutex_t lock;
    std::map<User*, MessageQueue*> members;
};

#endif
