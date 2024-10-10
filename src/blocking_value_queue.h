
#pragma once


namespace sorth
{


    class BlockingValueQueue
    {
        private:
            std::mutex item_lock;
            std::condition_variable condition;

            std::deque<Value> items;

        public:
            BlockingValueQueue();
            BlockingValueQueue(const BlockingValueQueue& queue);
            BlockingValueQueue(BlockingValueQueue&& queue);

        public:
            BlockingValueQueue& operator =(const BlockingValueQueue& queue);
            BlockingValueQueue& operator =(BlockingValueQueue&& queue);

        public:
            int64_t depth();

            void push(Value& value);
            Value pop();
    };


}
