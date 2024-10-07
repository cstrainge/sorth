
#include "sorth.h"


namespace sorth
{


    BlockingValueQueue::BlockingValueQueue()
    : item_lock(),
      condition(),
      items()
    {
    }


    BlockingValueQueue::BlockingValueQueue(const BlockingValueQueue& queue)
    : item_lock(),
      condition(),
      items()
    {
        std::lock_guard<std::mutex> lock(const_cast<BlockingValueQueue&>(queue).item_lock);

        items = queue.items;
    }


    BlockingValueQueue::BlockingValueQueue(BlockingValueQueue&& queue)
    : item_lock(),
      condition(),
      items()
    {
        std::lock_guard<std::mutex> lock(queue.item_lock);

        items = std::move(queue.items);
    }


    BlockingValueQueue& BlockingValueQueue::operator =(const BlockingValueQueue& queue)
    {
        if (&queue != this)
        {
            std::lock_guard<std::mutex> lock(const_cast<BlockingValueQueue&>(queue).item_lock);
            std::lock_guard<std::mutex> lock2(item_lock);

            items = queue.items;
        }

        return *this;
    }

    BlockingValueQueue& BlockingValueQueue::operator =(BlockingValueQueue&& queue)
    {
        if (&queue != this)
        {
            std::lock_guard<std::mutex> lock(queue.item_lock);
            std::lock_guard<std::mutex> lock2(item_lock);

            items = std::move(queue.items);
        }

        return *this;
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
        condition.notify_one();
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
