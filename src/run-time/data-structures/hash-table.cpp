
#include "sorth.h"



namespace sorth
{


    using namespace internal;


    std::ostream& operator <<(std::ostream& stream, const HashTablePtr& table)
    {
        stream << "{" << std::endl;

        Value::value_format_indent += 4;

        int index = 0;

        for (const auto& item : table->get_items())
        {
            stream << std::string(Value::value_format_indent, ' ');

            if (item.first.is_string())
            {
                stream << stringify(item.first);
            }
            else
            {
                stream << item.first;
            }

            stream << " -> ";

            if (item.second.is_string())
            {
                stream << stringify(item.second);
            }
            else
            {
                stream << item.second;
            }

            if (index < table->get_items().size() - 1)
            {
                stream << " ,";
            }

            stream << std::endl;

            ++index;
        }

        Value::value_format_indent -= 4;

        stream << std::string(Value::value_format_indent, ' ') << "}";


        return stream;
    }


    std::strong_ordering operator <=>(const HashTable& lhs, const HashTable& rhs)
    {
        if (lhs.items.size() != rhs.items.size())
        {
            return lhs.items.size() <=> rhs.items.size();
        }

        for (const auto& [ key, value ] : lhs.items)
        {
            auto iterator = rhs.items.find(key);

            if (iterator == rhs.items.end())
            {
                return std::strong_ordering::greater;
            }

            if (value != iterator->second)
            {
                return value <=> iterator->second;
            }
        }

        return std::strong_ordering::equal;
    }


    std::strong_ordering operator <=>(const HashTablePtr& lhs, const HashTablePtr& rhs)
    {
        return *lhs <=> *rhs;
    }


    HashTable::HashTable()
    {
    }


    int64_t HashTable::size() const
    {
        return items.size();
    }


    std::tuple<bool, Value> HashTable::get(const Value& key)
    {
        auto iter = items.find(key);

        if (iter == items.end())
        {
            return { false, {} };
        }

        return { true, iter->second };
    }


    void HashTable::insert(const Value& key, const Value& value)
    {
        auto iter = items.find(key);

        if (iter != items.end())
        {
            iter->second = value;
        }
        else
        {
            items.insert({ key, value });
        }
    }


    size_t HashTable::hash() const noexcept
    {
        size_t hash_value = 0;

        for (const auto& [ key, value ] : items)
        {
            Value::hash_combine(hash_value, key.hash());
            Value::hash_combine(hash_value, value.hash());
        }

        return hash_value;
    }


}
