
#include "sorth.h"


namespace sorth
{


    std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array)
    {
        stream << "[ ";

        for (auto item : array->items)
        {
            stream << item << " ";
        }

        stream << "]";

        return stream;
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


}