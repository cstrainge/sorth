
#include "sorth.h"


namespace sorth
{


    std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array)
    {
        stream << "[ ";

        for (size_t i = 0; i < array->items.size(); ++i)
        {
            stream << array->items[i];

            if (i < (array->items.size() - 1))
            {
                stream << " , ";
            }
        }

        stream << " ]";

        return stream;
    }


    bool operator ==(const ArrayPtr& rhs, const ArrayPtr& lhs)
    {
        if (rhs->size() != lhs->size())
        {
            return false;
        }

        for (int i = 0; i < rhs->size(); ++i)
        {
            if ((*rhs)[i] != (*lhs)[i])
            {
                return false;
            }
        }

        return true;
    }


    Array::Array(int64_t size)
    {
        items.resize(size);
    }


    int64_t Array::size() const
    {
        return items.size();
    }

    Value& Array::operator [](int64_t index)
    {
        return items[index];
    }

    void Array::resize(int64_t new_size)
    {
        items.resize((size_t)new_size);
    }

    void Array::insert(int64_t index, const Value& value)
    {
        items.insert(std::next(items.begin(), index), value);
    }

    void Array::remove(int64_t index)
    {
        items.erase(std::next(items.begin(), index));
    }

    void Array::push_front(const Value& value)
    {
        items.insert(items.begin(), value);
    }

    void Array::push_back(const Value& value)
    {
        items.push_back(value);
    }

    Value Array::pop_front(InterpreterPtr& interpreter)
    {
        if (items.empty())
        {
            internal::throw_error(interpreter, "Popping from an empty array.");
        }

        Value value = items[0];

        items.erase(items.begin());

        return value;
    }

    Value Array::pop_back(InterpreterPtr& interpreter)
    {
        if (items.empty())
        {
            internal::throw_error(interpreter, "Popping from an empty array.");
        }

        Value value = items[0];

        items.pop_back();

        return value;
    }


}
