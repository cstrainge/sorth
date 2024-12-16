
#pragma once


namespace sorth
{


    class Array
    {
        private:
            std::vector<Value> items;

        public:
            Array(size_t size);

        public:
            size_t size() const;
            Value& operator [](size_t index);
            void resize(size_t new_size);

            void insert(size_t index, const Value& value);
            void remove(size_t index);

        public:
            void push_front(const Value& value);
            void push_back(const Value& value);
            Value pop_front(InterpreterPtr& interpreter);
            Value pop_back(InterpreterPtr& interpreter);

        public:
            size_t hash() const noexcept;

        private:
            friend std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array);
            friend std::strong_ordering operator <=>(const Array& rhs, const Array& lhs);
    };


    std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array);


    std::strong_ordering operator <=>(const Array& rhs, const Array& lhs);

    std::strong_ordering operator <=>(const ArrayPtr& rhs, const ArrayPtr& lhs);


    inline bool operator ==(const ArrayPtr& rhs, const ArrayPtr& lhs)
    {
        return *rhs <=> *lhs == std::strong_ordering::equal;
    }


    inline bool operator !=(const ArrayPtr& rhs, const ArrayPtr& lhs)
    {
        return *rhs <=> *lhs != std::strong_ordering::equal;
    }


}
