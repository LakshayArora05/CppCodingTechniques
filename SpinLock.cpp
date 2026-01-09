#include <iostream>
#include <atomic>

class SpinLock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;  // Initially false
    
public:
    void lock() {
        bool was_already_locked = true;
        
        while (was_already_locked == true) {
            // Atomically: read old value, set to true, return old value.
            was_already_locked = flag.test_and_set(std::memory_order_acquire);            
            // If was_already_locked == false: we got the lock, exit loop.
            // If was_already_locked == true: someone else has it, keep spinning.
        }
        
        // When we exit loop, we have successfully acquired the lock
    }
    
    void unlock() {
        // Set flag back to false, releasing the lock
        flag.clear(std::memory_order_release);
    }
};
