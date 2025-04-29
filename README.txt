Nicholas Llaurado, Antony Goldenberg

Nicholas: Implemented and tested receiever and sender, and server.
Antony: Implemented connection and debugged receiever and sender. Implemented room and message queue.


Critical Sections and Synchronization:

In our project, critical sections are portions of code that involve access to shared data structures between multiple threads. 
To prevent data races, deadlocks, and ensure correct program behavior, we used synchronization techniques (mainly mutexes and guards)
to protect these areas. Our identified critical sections are outlined below:

Section 1: In server.cpp, when finding or creating a room.
    - Shared Data: The list of rooms accessed by multiple threads because multiple threads may call find_or_create_room when 
        clients try to join a room at the same time.
    - Synchronization: We use a mutex along with a Guard object to protect access. 
        The mutex protects against a data race if multiple threads try to call find_or_create_room at the same time
         , and the Guard prevents deadlocks by automatically unlocking at the end of the scope.

Section 2: In room.cpp, when adding a member to a room.
    - Shared Data: The room’s list of members because multiple clients may try to join/leave the room or send a message at the same time.
    - Synchronization: We use a mutex and Guard combination. The mutex allows ony one thread at a time to be added to and to interact with the room, 
         preventing data races. Also the guard prevents deadlocks by automatically unlocking the mutex at the end of the function scope.

Section 3: In room.cpp, when removing a member from a room.
    - Shared Data: The room’s list of members because it is shared member data and accessed by multiple threads. The same reasoning as above
        applies for the clients attempting to leave the room at the same time.
    - Synchronization: We use a mutex/Guard combo to make sure only one thread could remove a member at a time, preventing data races and the 
        guard prevents deadlocks by automatically unlocking at the end of the scope.

Section 4: In room.cpp, when broadcasting a message to all members.
    - Shared Data: The room's list of members again because actions such as sending a message to all members of a room deal with shared member data.
    - Synchronization: We use a mutex and Guard so that the broadcast operation could safely send a copy of the
         message to each client without risk of multiple threads modifying the same message simultaneously in a data race. Once again the guard
         prevents deadlocks by automatically unlocking at the end of the scope.

Section 5: In message_queue.cpp, in the enqueue method when adding a message to the queue.
    - Shared Data: The shared queue of messages because of the shared access to the queue of messages.
    - Synchronization: We use a mutex/Guard combo to ensure only one thread can push onto the queue at a time. If multiple threads tried to push
         onto the queue simultaneously, it could lead to a data race. The guard prevents deadlocks by automatically unlocking at the end of the scope.
         The call to sem post is used to signal that a message has been added to the queue, allowing other threads to continue processing.
       

Section 6: In message_queue.cpp, in the dequeue method when removing a message from the queue.
    - Shared Data: The shared queue of messages because of the shared access to the queue.
    - Synchronization: We use a block-scoped mutex/Guard combo to protect only the portion of code accessing the queue.
         We intentionally delayed locking the mutex until after sem_timedwait to maximize concurrency and avoid unnecessary blocking.
         The mutex ensures that only one thread can remove a message from the queue at a time, preventing data races. The guard prevents deadlocks by automatically unlocking at the end of the scope and 
         is initialized after sem_timedwait to avoid slowing the server down since waiting for the timed wait finish first lets the mutex encompass only the portion of code that accesses the queue.

Section 7: In message_queue.cpp, in the queue destructor when freeing all messages.
    - Shared Data: The message queue accessed by multiple threads because multiple threads may call the destructor at the same time.
    - Synchronization: We protect the messages with a mutex and Guard to safely delete each message, otherwrise deleting messages
    in parallel wuthout locking could lead to a data race. The guard prevents deadlocks by automatically unlocking at the end of the scope.
    The mutex is locked first to protect the data, and then the guard ensures the mutex is unlocked before the mutex is destroyed.