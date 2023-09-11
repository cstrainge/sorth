
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

        for (size_t i = 0; i < rhs->size(); ++i)
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


}
