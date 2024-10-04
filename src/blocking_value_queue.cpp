
#include "sorth.h"


namespace sorth
{


    BlockingValueQueue::BlockingValueQueue()
    : item_lock(),
      condition(),
      items()
    {
    }


    BlockingValueQueue::BlockingValueQueue(const BlockingValueQueue& stack)
    : item_lock(),
      condition(),
      items(stack.items)
    {
    }


    BlockingValueQueue::BlockingValueQueue(BlockingValueQueue&& stack)
    : item_lock(),
      condition(),
      items(std::move(stack.items))
    {
    }


    int64_t BlockingValueQueue::depth()
    {
        std::lock_guard<std::mutex> lock(item_lock);
        return items.size();
    }


    void BlockingValueQueue::push(Value& value)
    {
        std::lock_guard<std::mutex> lock(item_lock);

        items.push_front(value);
        condition.notify_all();
    }


    Value BlockingValueQueue::pop()
    {
        std::unique_lock<std::mutex> lock(item_lock);

        condition.wait(lock, [this]() { return !items.empty(); });

        auto value = items.back();
        items.pop_back();

        return value;
    }


}
