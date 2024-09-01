
#include "sorth.h"



namespace sorth
{


    using namespace internal;


    std::ostream& operator <<(std::ostream& stream, const HashTablePtr& table)
    {
        static thread_local int indent = 0;

        auto write_indent = [&]()
            {
                for (int i = 0; i < indent; ++i)
                {
                    stream << "    ";
                }
            };

        ++indent;

        stream << "{" << std::endl;

        for (const auto& item : table->get_items())
        {
            write_indent();

            stream << item.first << " -> " << item.second << std::endl;
        }

        --indent;

        write_indent();

        stream << "}";


        return stream;
    }


    bool operator ==(const HashTablePtr& rhs, const HashTablePtr& lhs)
    {
        auto& r_rhs = *rhs;
        auto& r_lhs = *lhs;

        if (r_rhs.size() != r_lhs.size())
        {
            return false;
        }

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
