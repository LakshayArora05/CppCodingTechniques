
#include <iostream>
#include <mutex>
#include <condition_variable>

template <typename T>
class  CustomQueue
{
    private:
        struct Element
        {
            T data;
            Element* next;
        };
        
        Element* Head;
        Element* Tail;
        std::mutex m;
        std::condition_variable cv; 
    
    public:
        CustomQueue() : Head(nullptr), Tail(nullptr) {}
        CustomQueue(const CustomQueue&) = delete;
        CustomQueue& operator=(const CustomQueue&) = delete;
        ~CustomQueue() {
        while (Head != nullptr)
            {
                Element* temp = Head;
                Head = Head->next;
                delete temp;
            }
    }
        void push(T data)
        {
            std::lock_guard<std::mutex> lock(m);
            Element* newElement = new Element{data,nullptr};
            if(Head==nullptr)
            {   
                Head =newElement;
                Tail=newElement;
            }
            
            else
            {
                Tail->next =newElement;
                Tail= newElement;
            }
            cv.notify_one();
        }
        
        T pop()
        {
            std::unique_lock<std::mutex> lock(m);
              cv.wait(lock, [this]{ return Head != nullptr; });
               T datareturned = Head->data;
               Element* oldelement = Head;
               Head= Head->next;  
               
               if(Head ==nullptr)
               {
                   Tail= nullptr;
               }
            
            delete oldelement;
            return datareturned;
        }
        
    
};
