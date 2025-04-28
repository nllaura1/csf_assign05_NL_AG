#include <cassert>
#include <ctime>
#include "message_queue.h"
#include "message.h"
#include "guard.h"

// Constructor for MessageQueue
MessageQueue::MessageQueue() {
    // Initialize the mutex lock for thread safety
    pthread_mutex_init(&m_lock, nullptr);
    // Initialize the semaphore to track available messages (starting with 0)
    sem_init(&m_avail, 0, 0);
}

// Destructor for MessageQueue
MessageQueue::~MessageQueue() {
    // Guard to automatically lock/unlock the mutex
    Guard guard(m_lock);

    // Clean up all remaining messages in the queue
    while (!m_messages.empty()) {
        delete m_messages.front();  // Free memory for each message
        m_messages.pop_front();    // Remove from queue
    }
    // Destroy the mutex and semaphore
    pthread_mutex_destroy(&m_lock);
    sem_destroy(&m_avail);
}

// Add a message to the queue
void MessageQueue::enqueue(Message *msg) {
    // Use a Guard to automatically lock/unlock the mutex
    Guard guard(m_lock);
    // Add the message to the end of the queue
    m_messages.push_back(msg);
    //printf("[queue] Enqueued message: %s\n", msg->data.c_str());

    // Increment the semaphore to indicate a new message is available
    sem_post(&m_avail);
}

// Remove and return a message from the queue
Message *MessageQueue::dequeue() {
    // Set up a timeout of 1 second for the semaphore wait
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;  // Set timeout to 1 second from now

    // Wait for a message to be available, with timeout
    if (sem_timedwait(&m_avail, &ts) != 0) {
        // Return nullptr if timeout occurs
        return nullptr; 
    }

    // Use a Guard to automatically lock/unlock the mutex
    Guard guard(m_lock);
    // Checknin case the queue is empty (shouldn't happen bc of semaphore)
    if (m_messages.empty()) {
        return nullptr;
    }
    // Get the first message from the queue
    Message* msg = m_messages.front();
    m_messages.pop_front();  // Remove it from the queue
    return msg;              // Return the message
}