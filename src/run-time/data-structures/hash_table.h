
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

        private:
            friend std::ostream& operator <<(std::ostream& stream, const ArrayPtr& table);
            friend bool operator ==(const HashTablePtr& rhs, const HashTablePtr& lhs);
    };


    std::ostream& operator <<(std::ostream& stream, const HashTablePtr& table);

    bool operator ==(const HashTablePtr& rhs, const HashTablePtr& lhs);


}
