
#pragma once


namespace sorth
{


    class BlockingValueQueue
    {
        private:
            std::mutex item_lock;
            std::condition_variable condition;

            std::list<Value> items;

        public:
            BlockingValueQueue();
            BlockingValueQueue(const BlockingValueQueue& stack);
            BlockingValueQueue(BlockingValueQueue&& stack);

        public:
            int64_t depth();

            void push(Value& value);
            Value pop();
    };


}
