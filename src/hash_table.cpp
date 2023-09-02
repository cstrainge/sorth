
#include "sorth.h"



namespace sorth
{


    using namespace internal;


    std::ostream& operator <<(std::ostream& stream, const HashTablePtr& table)
    {
        stream << "{" << std::endl;

        for (const auto& item : table->get_items())
        {
            stream << "    " << item.first << " -> " << item.second << std::endl;
        }

        stream << "}";

        return stream;
    }


    bool operator ==(const HashTablePtr& rhs, const HashTablePtr& lhs)
    {
        if (rhs == lhs)
        {
            return true;
        }

        auto& r_rhs = *rhs;
        auto& r_lhs = *lhs;

        for (auto iter : r_rhs.items)
        {
            auto found_iter = r_lhs.items.find(iter.first);

            if (found_iter == r_rhs.items.end())
            {
                return false;
            }

            if (iter.second != found_iter->second)
            {
                return false;
            }
        }

        return true;
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


}
