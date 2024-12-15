
#pragma once


namespace sorth
{


    class Array
    {
        private:
            std::vector<Value> items;

        public:
            Array(int64_t size);

        public:
            int64_t size() const;
            Value& operator [](int64_t index);
            void resize(int64_t new_size);

            void insert(int64_t index, const Value& value);
            void remove(int64_t index);

        public:
            void push_front(const Value& value);
            void push_back(const Value& value);
            Value pop_front(InterpreterPtr& interpreter);
            Value pop_back(InterpreterPtr& interpreter);

        private:
            friend std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array);
            friend bool operator ==(const ArrayPtr& rhs, const ArrayPtr& lhs);
    };


    std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array);

    bool operator ==(const ArrayPtr& rhs, const ArrayPtr& lhs);


}
