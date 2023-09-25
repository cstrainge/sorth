
#include "sorth.h"


namespace sorth
{


    std::ostream& operator <<(std::ostream& stream, const DataObjectPtr& data)
    {
        if (data)
        {
            stream << "# " << data->definition->name << " ";

            for (size_t i = 0; i < data->fields.size(); ++i)
            {
                stream << data->definition->fieldNames[i] << ": "
                       << data->fields[i] << " ";
            }

            stream << ";";
        }
        else
        {
            stream << "NULL";
        }

        return stream;
    }


    bool operator ==(const DataObjectPtr& rhs, const DataObjectPtr& lhs)
    {
        if (rhs->definition->name != lhs->definition->name)
        {
            return false;
        }

        for (size_t i = 0; i < rhs->fields.size(); ++i)
        {
            if (rhs->fields[i] != lhs->fields[i])
            {
                return false;
            }
        }

        return true;
    }


}
