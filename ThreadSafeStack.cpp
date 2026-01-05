#include <iostream>
#include <stack>
#include <mutex>
#include <stdexcept>
#include <string>

template<typename T>
class ThreadSafeStack {
private:
    std::stack<T> data;
    mutable std::mutex m;  //  kept mutable,  allows locking in const functions
    
public:
    ThreadSafeStack() {}    
    ThreadSafeStack(const ThreadSafeStack& other) {                   // Copy constructor - locks the source stack during copy.
        std::lock_guard<std::mutex> lock(other.m);
        data = other.data;
    }
    
   
    ThreadSafeStack& operator=(const ThreadSafeStack&) = delete;      // Assignment operator deleted - prevents mutex assignment issues
    
    void push(T value) {
        std::lock_guard<std::mutex> lock(m);
        data.push(value);
    }
    
    T pop() {
        std::lock_guard<std::mutex> lock(m);  
        if(data.empty()) {
            throw std::runtime_error("ThreadSafeStack: pop() called on empty stack");
        }
        T val = data.top();
        data.pop();
        return val;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(m);
        return data.size();
    }
};
