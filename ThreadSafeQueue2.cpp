#include <iostream>
#include <mutex>
#include <condition_variable>

template <typename T>
class CustomQueue
{
private:
    struct Element
    {
        T data;           // Could use std::shared_ptr<T> for exception safety
        Element* next;    // Could use std::unique_ptr<Element> for automatic cleanup
    };
    
    Element* Head;  // Raw pointer - consider unique_ptr for RAII
    Element* Tail;  // Raw pointer - fine since owned by chain from Head
    
    std::mutex HeadMutex;  // Separate mutex for head - enables concurrent push/pop
    std::mutex TailMutex;  // Separate mutex for tail - key to fine-grained locking
    std::condition_variable cv;  // For blocking wait when queue empty
    
    Element* get_tail()  // Helper to minimize TailMutex lock scope
    {
        std::lock_guard<std::mutex> tail_lock(TailMutex);  // Lock only for read
        return Tail;  // Brief lock - better than holding throughout entire pop()
    }

public:
    CustomQueue() 
    {
        Element* dummy = new Element();  // Dummy node ensures Head != Tail always
        Head = dummy;  // Both point to dummy initially
        Tail = dummy;  // This separation prevents race conditions
        // Why dummy? So push (touches Tail) and pop (touches Head) never conflict
    }
    
    CustomQueue(const CustomQueue&) = delete;  // Non-copyable - mutexes can't be copied
    CustomQueue& operator=(const CustomQueue&) = delete;
    
    ~CustomQueue() 
    {
        while (Head != nullptr)  // Clean up all remaining nodes
        {
            Element* temp = Head;
            Head = Head->next;
            delete temp;  // Manual cleanup - could use unique_ptr to avoid this
        }
    }
    
    void push(T data)
    {
        // Optimization: Allocate new dummy OUTSIDE lock - reduces critical section
        Element* newdummy = new Element();
        newdummy->next = nullptr;
        
        {
            std::lock_guard<std::mutex> tail_lock(TailMutex);  // Lock only tail region
            // Only TailMutex locked - Head operations can proceed concurrently!
            
            Tail->data = data;        // Put data in old dummy (converts to real node)
            Tail->next = newdummy;    // Link new dummy after it
            Tail = newdummy;          // Tail always points to dummy
            
            // Why this pattern? Push never touches Head, so no contention with pop()
        }  // Unlock BEFORE notify - optimization for waiting threads
        
        cv.notify_one();  // Wake one waiting pop() thread if any
        // Notify outside lock = waiting thread can immediately grab TailMutex in get_tail()
    }
    
    T pop()  // Blocking pop - waits if empty
    {
        std::unique_lock<std::mutex> head_lock(HeadMutex);  // unique_lock for cv.wait()
        // Only HeadMutex locked - push operations can proceed concurrently!
        
        cv.wait(head_lock, [this]{ 
            return Head != get_tail();  // Predicate: queue not empty
            // get_tail() briefly locks TailMutex just to read Tail pointer
            // This is why we unlock before notify in push() - reduces contention here
        });
        
        // At this point: Head != Tail guaranteed, queue has data
        
        T datareturned = Head->data;  // Copy data out
        // Optimization: Could return std::move(Head->data) for move-only types
        // Exception risk: If T's copy throws, we've already removed from queue
        // solution: use shared_ptr<T> to avoid copy
        
        Element* oldelement = Head;  // Save old head for deletion
        Head = Head->next;  // Move Head forward - just pointer reassignment
        
        delete oldelement;  // Free old node
        // Optimization: Use unique_ptr to avoid manual delete
        
        return datareturned;
    }
    
    bool try_pop(T& value)  // Non-blocking pop - returns immediately
    {
        std::lock_guard<std::mutex> head_lock(HeadMutex);  // lock_guard (no cv.wait)
        
        if(Head == get_tail())  // Check if empty
        {
            return false;  // Queue empty, return immediately (non-blocking)
        }
        
        value = Head->data;  // Copy to reference parameter
        // Optimization: Could use std::move for move-only types
        
        Element* oldelement = Head;
        Head = Head->next;  // Advance Head
        
        delete oldelement;
        return true;  // Success
    }
    
    bool empty()
    {
        std::lock_guard<std::mutex> head_lock(HeadMutex);  // Lock to safely read Head
        return Head == get_tail();  // Empty when both point to dummy
        // get_tail() briefly locks TailMutex - very short lock duration
    }
};
