
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void word_data_definition(InterpreterPtr& interpreter)
        {
            Location location = interpreter->get_current_location();

            bool found_initializers = interpreter->pop_as_bool();
            bool is_hidden = interpreter->pop_as_bool();
            ArrayPtr fields = interpreter->pop_as_array();
            std::string name = interpreter->pop_as_string();
            ArrayPtr defaults;

            if (found_initializers)
            {
                defaults = interpreter->pop_as_array();
            }

            // Create the definition object.
            auto definition_ptr = create_data_definition(interpreter,
                                                         name,
                                                         fields,
                                                         defaults,
                                                         is_hidden ? WordVisibility::hidden
                                                                   : WordVisibility::visible);

            // Create the words to allow the script to access this definition.  The word
            // <definition_name>.new will always hold a base reference to our definition object.
            create_data_definition_words(location,
                                         interpreter,
                                         definition_ptr,
                                         is_hidden ? WordVisibility::hidden
                                                   : WordVisibility::visible);
        }


        void word_read_field(InterpreterPtr& interpreter)
        {
            auto object = interpreter->pop_as_structure();
            auto field_index = interpreter->pop_as_size();

            interpreter->push(object->fields[field_index]);
        }


        void word_write_field(InterpreterPtr& interpreter)
        {
            auto object = interpreter->pop_as_structure();
            auto field_index = interpreter->pop_as_size();

            object->fields[field_index] = interpreter->pop();
        }


        void word_structure_iterate(InterpreterPtr& interpreter)
        {
            auto object = interpreter->pop_as_structure();
            auto word_index = interpreter->pop_as_size();

            auto& handler = interpreter->get_handler_info(word_index);

            auto& data_type = object->definition;

            for (size_t i = 0; i < data_type->fieldNames.size(); ++i)
            {
                interpreter->push(data_type->fieldNames[i]);
                interpreter->push(object->fields[i]);

                handler.function(interpreter);
            }
        }


        void word_structure_field_exists(InterpreterPtr& interpreter)
        {
            auto object = interpreter->pop_as_structure();
            auto field_name = interpreter->pop_as_string();

            bool found = false;

            for (const auto& name : object->definition->fieldNames)
            {
                if (name == field_name)
                {
                    found = true;
                    break;
                }
            }

            interpreter->push(found);
        }


        void word_structure_compare(InterpreterPtr& interpreter)
        {
            auto a = interpreter->pop_as_structure();
            auto b = interpreter->pop_as_structure();

            interpreter->push(a == b);
        }


    }


    void register_structure_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "#", word_data_definition,
            "Beginning of a structure definition.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "#@", word_read_field,
            "Read a field from a structure.",
            "field_index structure -- value");

        ADD_NATIVE_WORD(interpreter, "#!", word_write_field,
            "Write to a field of a structure.",
            "value field_index structure -- ");

        ADD_NATIVE_WORD(interpreter, "#.iterate", word_structure_iterate,
            "Call an iterator for each member of a structure.",
            "word_or_index -- ");

        ADD_NATIVE_WORD(interpreter, "#.field-exists?", word_structure_field_exists,
            "Check if the named structure field exits.",
            "field_name structure -- boolean");

        ADD_NATIVE_WORD(interpreter, "#.=", word_structure_compare,
            "Check if two structures are the same.",
            "a b -- boolean");
    }


}
