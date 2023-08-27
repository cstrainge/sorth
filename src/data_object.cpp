
#include "sorth.h"


namespace sorth
{


    std::ostream& operator <<(std::ostream& stream, const DataObjectPtr& data)
    {
        if (data)
        {
            stream << "# " << data->definition->name << " ";

            for (int64_t i = 0; i < data->fields.size(); ++i)
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


}