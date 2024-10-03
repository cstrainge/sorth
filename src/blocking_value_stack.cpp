
#include "sorth.h"


namespace sorth
{


    BlockingValueStack::BlockingValueStack()
    : item_lock(),
      condition(),
      items()
    {
    }


    BlockingValueStack::BlockingValueStack(const BlockingValueStack& stack)
    : item_lock(),
      condition(),
      items(stack.items)
    {
    }


    BlockingValueStack::BlockingValueStack(BlockingValueStack&& stack)
    : item_lock(),
      condition(),
      items(std::move(stack.items))
    {
    }


    int64_t BlockingValueStack::depth()
    {
        std::lock_guard<std::mutex> lock(item_lock);
        return items.size();
    }


    void BlockingValueStack::push(Value& value)
    {
        std::lock_guard<std::mutex> lock(item_lock);

        items.push_front(value);
        condition.notify_all();
    }


    Value BlockingValueStack::pop()
    {
        std::unique_lock<std::mutex> lock(item_lock);

        condition.wait(lock, [this]() { return !items.empty(); });

        auto value = items.front();
        items.pop_front();

        return value;
    }


}
