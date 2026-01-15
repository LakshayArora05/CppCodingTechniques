#include <iostream>
#include <mutex>

template <typename T>
class ThreadSafeStack
{
private:
    struct Element
    {
        T data;
        Element* next;
    };
    
    Element* Top;  // Stack uses Top instead of Head (but same thing)
    std::mutex Mutex;

public:
    ThreadSafeStack() : Top(nullptr) {}
    
    ThreadSafeStack(const ThreadSafeStack&) = delete;
    ThreadSafeStack& operator=(const ThreadSafeStack&) = delete;
    
    ~ThreadSafeStack()
    {
        while (Top != nullptr)
        {
            Element* temp = Top;
            Top = Top->next;
            delete temp;
        }
    }
    
    void push(T value)
    {
        Element* newElement = new Element();
        newElement->data = value;
        
        std::lock_guard<std::mutex> lock(Mutex);
        
        newElement->next = Top;  // New element points to old top
        Top = newElement;        // Top now points to new element
        
        // Why this works: Stack always inserts at Top (front)
    }
    
    T pop()
    {
        std::lock_guard<std::mutex> lock(Mutex);
        
        if (Top == nullptr)
        {
            throw std::runtime_error("Stack is empty");
        }
        
        T datareturned = Top->data;  // Get data from top element
        Element* oldelement = Top;    // Save old top
        Top = Top->next;              // Move top to next element
        
        delete oldelement;  // Delete old top
        return datareturned;
        
    }
    
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lock(Mutex);
        
        if (Top == nullptr)
        {
            return false;  // Stack empty
        }
        
        value = Top->data;
        Element* oldelement = Top;
        Top = Top->next;
        
        delete oldelement;
        return true;
    }
    
    // Peek - look at top without removing
    T peek()
    {
        std::lock_guard<std::mutex> lock(Mutex);
        
        if (Top == nullptr)
        {
            throw std::runtime_error("Stack is empty");
        }
        
        return Top->data;  // Just read, don't modify
    }
    
    bool empty()
    {
        std::lock_guard<std::mutex> lock(Mutex);
        return Top == nullptr;
    }
    
};
