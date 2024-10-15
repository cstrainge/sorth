
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


    DataObjectDefinitionPtr create_data_definition(InterpreterPtr& interpreter,
                                                   std::string name,
                                                   ArrayPtr fields,
                                                   ArrayPtr defaults,
                                                   bool is_hidden)
    {
        DataObjectDefinitionPtr definition_ptr = std::make_shared<DataObjectDefinition>();

        definition_ptr->name = name;

        definition_ptr->fieldNames.resize(fields->size());
        definition_ptr->defaults.resize(fields->size());

        for (int i = 0; i < fields->size(); ++i)
        {
            definition_ptr->fieldNames[i] = as_string(interpreter, (*fields)[i]);

            if (defaults)
            {
                definition_ptr->defaults[i] = (*defaults)[i];
            }
        }

        return definition_ptr;
    }


    void create_data_definition_words(const internal::Location &location,
                                      InterpreterPtr &interpreter,
                                      DataObjectDefinitionPtr &definition_ptr,
                                      bool is_hidden)
    {
        const bool is_immediate = false;
        const bool is_scripted = false;

        interpreter->add_word(definition_ptr->name + ".new",
            [=](auto interpreter)
            {
                auto new_object = make_data_object(interpreter, definition_ptr);
                interpreter->push(new_object);
            },
            location,
            is_immediate,
            is_hidden,
            is_scripted,
            "Create a new instance of the structure " + definition_ptr->name + ".",
            " -- " + definition_ptr->name);

        auto swap_tuple = interpreter->find_word("swap");
        auto struct_write_tuple = interpreter->find_word("#!");
        auto struct_read_tuple = interpreter->find_word("#@");

        // Work around for older compilers that can't handle destructured variables being passed
        // into a lambda function.
        auto swap_found = std::get<0>(swap_tuple);
        auto swap = std::get<1>(swap_tuple);

        auto struct_write_found = std::get<0>(struct_write_tuple);
        auto struct_write = std::get<1>(struct_write_tuple);

        auto struct_read_found = std::get<0>(struct_read_tuple);
        auto struct_read = std::get<1>(struct_read_tuple);

        for (int64_t i = 0; i < (int64_t)definition_ptr->fieldNames.size(); ++i)
        {
            interpreter->add_word(definition_ptr->name + "." + definition_ptr->fieldNames[i],
                [i](auto interpreter)
                {
                    interpreter->push(i);
                },
                location,
                is_immediate,
                is_hidden,
                is_scripted,
                "Access the structure field " + definition_ptr->fieldNames[i] + ".",
                " -- structure_field_index");

            if (swap_found && struct_write_found && struct_read_found)
            {
                auto field_writer = [i, swap, struct_write](auto interpreter)
                    {
                        interpreter->push(i);
                        interpreter->execute_word(swap);
                        interpreter->execute_word(struct_write);
                    };

                auto field_reader = [i, swap, struct_read](auto interpreter)
                    {
                        interpreter->push(i);
                        interpreter->execute_word(swap);
                        interpreter->execute_word(struct_read);
                    };

                auto var_field_writer = [i, swap, struct_write](auto interpreter)
                    {
                        interpreter->push(i);
                        interpreter->execute_word(swap);

                        auto variables = interpreter->get_variables();
                        auto index = as_numeric<int64_t>(interpreter, interpreter->pop());
                        interpreter->push(variables[index]);

                        interpreter->execute_word(struct_write);
                    };

                auto var_field_reader = [i, swap, struct_read](auto interpreter)
                    {
                        interpreter->push(i);
                        interpreter->execute_word(swap);

                        auto variables = interpreter->get_variables();
                        auto index = as_numeric<int64_t>(interpreter, interpreter->pop());
                        interpreter->push(variables[index]);

                        interpreter->execute_word(struct_read);
                    };

                interpreter->add_word(
                    definition_ptr->name + "." + definition_ptr->fieldNames[i] + "!",
                    field_writer,
                    location,
                    is_immediate,
                    is_hidden,
                    is_scripted,
                    "Write to the structure field " + definition_ptr->fieldNames[i] + ".",
                    "new_value structure -- ");

                interpreter->add_word(
                    definition_ptr->name + "." + definition_ptr->fieldNames[i] + "@",
                    field_reader,
                    location,
                    is_immediate,
                    is_hidden,
                    is_scripted,
                    "Read from structure field " + definition_ptr->fieldNames[i] + ".",
                    "structure -- value");

                interpreter->add_word(
                    definition_ptr->name + "." + definition_ptr->fieldNames[i] + "!!",
                    var_field_writer,
                    location,
                    is_immediate,
                    is_hidden,
                    is_scripted,
                    "Write to the structure field " + definition_ptr->fieldNames[i] +
                        " in a variable.",
                    "new_value structure_var -- ");

                interpreter->add_word(
                    definition_ptr->name + "." + definition_ptr->fieldNames[i] + "@@",
                    var_field_reader,
                    location,
                    is_immediate,
                    is_hidden,
                    is_scripted,
                    "Read from the structure field " + definition_ptr->fieldNames[i] +
                        " in a variable.",
                    "structure_var -- value");
            }
        }
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
