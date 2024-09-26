
#include "sorth.h"



namespace sorth
{


    using namespace internal;


    std::ostream& operator <<(std::ostream& stream, const HashTablePtr& table)
    {
        stream << "{" << std::endl;

        value_print_indent += 4;

        int index = 0;

        for (const auto& item : table->get_items())
        {
            stream << std::string(value_print_indent, ' ');

            if (std::holds_alternative<std::string>(item.first))
            {
                stream << stringify(item.first);
            }
            else
            {
                stream << item.first;
            }

            stream << " -> ";

            if (std::holds_alternative<std::string>(item.second))
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

        value_print_indent -= 4;

        stream << std::string(value_print_indent, ' ') << "}";


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
