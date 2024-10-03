
#pragma once


namespace sorth
{


    class BlockingValueStack
    {
        private:
            std::mutex item_lock;
            std::condition_variable condition;

            std::list<Value> items;

        public:
            BlockingValueStack();
            BlockingValueStack(const BlockingValueStack& stack);
            BlockingValueStack(BlockingValueStack&& stack);

        public:
            int64_t depth();

            void push(Value& value);
            Value pop();
    };


}
