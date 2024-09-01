
#include "sorth.h"


namespace sorth
{


    std::ostream& operator <<(std::ostream& stream, const DataObjectPtr& data)
    {
        static thread_local uint64_t indent = 4;

        if (data)
        {
            stream << "# " << data->definition->name << "\n";

            for (size_t i = 0; i < data->fields.size(); ++i)
            {
                stream << std::string(indent, ' ') << data->definition->fieldNames[i] << " -> ";

                indent += 4;
                stream << data->fields[i];
                indent -= 4;

                if (i < data->fields.size() - 1)
                {
                    stream << " ,\n";
                }
            }

            if (indent > 4)
            {
                stream << "\n" << std::string(indent - 4, ' ') << ";";
            }
            else
            {
                stream << "\n;" << std::endl;
            }
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
