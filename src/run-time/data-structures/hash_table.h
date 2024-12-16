
#pragma once


namespace sorth
{


    class HashTable
    {
        private:
            std::unordered_map<Value, Value> items;

        public:
            HashTable();

        public:
            int64_t size() const;

            std::tuple<bool, Value> get(const Value& key);
            void insert(const Value& key, const Value& value);

            const std::unordered_map<Value, Value>& get_items() const
            {
                return items;
            }

        public:
            size_t hash() const noexcept;

        private:
            friend std::ostream& operator <<(std::ostream& stream, const ArrayPtr& table);
            friend std::strong_ordering operator <=>(const HashTable& rhs, const HashTable& lhs);
    };


    std::ostream& operator <<(std::ostream& stream, const HashTablePtr& table);


    std::strong_ordering operator <=>(const HashTable& rhs, const HashTable& lhs);

    std::strong_ordering operator <=>(const HashTablePtr& rhs, const HashTablePtr& lhs);


    inline bool operator ==(const HashTable& rhs, const HashTable& lhs)
    {
        return (rhs <=> lhs) == std::strong_ordering::equal;
    }


    inline bool operator !=(const HashTable& rhs, const HashTable& lhs)
    {
        return (rhs <=> lhs) != std::strong_ordering::equal;
    }


    inline bool operator ==(const HashTablePtr& rhs, const HashTablePtr& lhs)
    {
        return *rhs == *lhs;
    }


    inline bool operator !=(const HashTablePtr& rhs, const HashTablePtr& lhs)
    {
        return *rhs == *lhs;
    }


}
