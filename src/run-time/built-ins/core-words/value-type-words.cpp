
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void word_value_is_number(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            interpreter->push(value.is_numeric());
        }


        void word_value_is_boolean(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            interpreter->push(value.is_bool());
        }


        void word_value_is_string(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            interpreter->push(value.is_string());
        }


        void word_value_is_thread_id(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            interpreter->push(value.is_thread_id());
        }


        void word_value_is_structure(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            interpreter->push(value.is_structure());
        }


        void word_value_is_array(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            interpreter->push(value.is_array());
        }


        void word_value_is_buffer(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            interpreter->push(value.is_byte_buffer());
        }


        void word_value_is_hash_table(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            interpreter->push(value.is_hash_table());
        }


        void word_value_copy(InterpreterPtr& interpreter)
        {
            auto original = interpreter->pop();

            interpreter->push(original.deep_copy());
        }


    }


    void register_value_type_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "value.is-number?", word_value_is_number,
            "Is the value a number?",
            "value -- bool");

        ADD_NATIVE_WORD(interpreter, "value.is-boolean?", word_value_is_boolean,
            "Is the value a boolean?",
            "value -- bool");

        ADD_NATIVE_WORD(interpreter, "value.is-string?", word_value_is_string,
            "Is the value a string?",
            "value -- bool");

        ADD_NATIVE_WORD(interpreter, "value.is-structure?", word_value_is_structure,
            "Is the value a structure?",
            "value -- bool");

        ADD_NATIVE_WORD(interpreter, "value.is-array?", word_value_is_array,
            "Is the value an array?",
            "value -- bool");

        ADD_NATIVE_WORD(interpreter, "value.is-buffer?", word_value_is_buffer,
            "Is the value a byte buffer?",
            "value -- bool");

        ADD_NATIVE_WORD(interpreter, "value.is-hash-table?", word_value_is_hash_table,
            "Is the value a hash table?",
            "value -- bool");

        ADD_NATIVE_WORD(interpreter, "value.copy", word_value_copy,
            "Create a new value that's a copy of another.  Deep copy as required.",
            "value -- new_copy");
    }


}
