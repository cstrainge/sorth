
#include "sorth.h"


namespace sorth
{


    std::ostream& operator <<(std::ostream& stream, const DataObjectPtr& data)
    {
        if (data)
        {
            stream << "# " << data->definition->name << "\n";

            value_print_indent += 4;

            for (size_t i = 0; i < data->fields.size(); ++i)
            {
                stream << std::string(value_print_indent, ' ')
                       << data->definition->fieldNames[i] << " -> ";

                if (std::holds_alternative<std::string>(data->fields[i]))
                {
                    stream << stringify(data->fields[i]);
                }
                else
                {
                    stream << data->fields[i];
                }

                if (i < data->fields.size() - 1)
                {
                    stream << " ,\n";
                }
            }

            value_print_indent -= 4;

            stream << "\n" << std::string(value_print_indent, ' ') << ";";
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


    DataObjectPtr make_data_object(InterpreterPtr& interpreter,
                                   const DataObjectDefinitionPtr& definition)
    {
        auto new_data = std::make_shared<DataObject>();

        new_data->definition = definition;
        new_data->fields.resize(definition->fieldNames.size());

        for (int index = 0; index < definition->defaults.size(); ++index)
        {
            new_data->fields[index] = deep_copy_value(interpreter, definition->defaults[index]);
        }

        return new_data;
    }


}
