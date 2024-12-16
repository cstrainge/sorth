
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void word_string_length(InterpreterPtr& interpreter)
        {
            auto string = interpreter->pop_as_string();

            interpreter->push((int64_t)string.size());
        }


        void word_string_insert(InterpreterPtr& interpreter)
        {
            auto string = interpreter->pop_as_string();
            auto position = interpreter->pop_as_size();
            auto sub_string = interpreter->pop_as_string();

            string.insert(position, sub_string);

            interpreter->push(string);
        }


        void word_string_remove(InterpreterPtr& interpreter)
        {
            auto string = interpreter->pop_as_string();
            auto start = interpreter->pop_as_size();
            auto count = interpreter->pop_as_size();

            if (   (start < 0)
                || (start > string.size()))
            {
                std::stringstream message;

                message << "string.remove start index, " << start << ", outside of the string.";
                throw_error(message.str());
            }

            if (   (count != std::string::npos)
                && ((start + count) > string.size()))
            {
                std::stringstream message;

                message << "string.remove end index, " << start + count << ", outside of the string.";
                throw_error(message.str());
            }

            string.erase(start, count);

            interpreter->push(string);
        }


        void word_string_find(InterpreterPtr& interpreter)
        {
            auto string = interpreter->pop_as_string();
            auto search_str = interpreter->pop_as_string();

            interpreter->push((int64_t)string.find(search_str, 0));
        }


        void word_string_index_read(InterpreterPtr& interpreter)
        {
            auto string = interpreter->pop_as_string();
            auto position = interpreter->pop_as_size();

            if ((position < 0) || (position >= string.size()))
            {
                throw_error(interpreter, "String index out of range.");
            }

            std::string new_str(1, string[position]);
            interpreter->push(new_str);
        }


        void word_string_add(InterpreterPtr& interpreter)
        {
            auto str_b = interpreter->pop_as_string();
            auto str_a = interpreter->pop_as_string();

            interpreter->push(str_a + str_b);
        }


        void word_string_to_number(InterpreterPtr& interpreter)
        {
            auto string = interpreter->pop_as_string();

            if (string.find('.', 0) != std::string::npos)
            {
                interpreter->push(std::atof(string.c_str()));
            }
            else
            {
                interpreter->push((int64_t)std::strtoll(string.c_str(), nullptr, 10));
            }
        }


        void word_to_string(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();
            std::stringstream stream;

            stream << value;
            interpreter->push(stream.str());
        }


        void word_hex(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            std::stringstream stream;

            if (value.is_string())
            {
                auto string_value = value.as_string(interpreter);

                for (auto next : string_value)
                {
                    stream << std::hex << (int)next << std::dec;
                }
            }
            else
            {
                auto int_value = value.as_integer(interpreter);
                stream << std::hex << int_value << std::dec << " ";
            }

            interpreter->push(stream.str());
        }


    }


    void register_string_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "string.size@", word_string_length,
            "Get the length of a given string.",
            "string -- size");

        ADD_NATIVE_WORD(interpreter, "string.[]!", word_string_insert,
            "Insert a string into another string.",
            "sub_string position string -- updated_string");

        ADD_NATIVE_WORD(interpreter, "string.remove", word_string_remove,
            "Remove some characters from a string.",
            "count position string -- updated_string");

        ADD_NATIVE_WORD(interpreter, "string.find", word_string_find,
            "Find the first instance of a string within another.",
            "search_string string -- index");

        ADD_NATIVE_WORD(interpreter, "string.[]@", word_string_index_read,
            "Read a character from the given string.",
            "index string -- character");

        ADD_NATIVE_WORD(interpreter, "string.+", word_string_add,
            "Add a string onto the end of another.",
            "str_a str_b -- new_str");

        ADD_NATIVE_WORD(interpreter, "string.to_number", word_string_to_number,
            "Convert a string into a number.",
            "string -- number");

        ADD_NATIVE_WORD(interpreter, "value.to-string", word_to_string,
            "Convert a value to a string.",
            "value -- string");

        ADD_NATIVE_WORD(interpreter, "string.npos", [](auto& interpreter)
            {
                interpreter->push((int64_t)std::string::npos);
            },
            "Constant value that indicates a search has failed.",
            " -- npos");

        ADD_NATIVE_WORD(interpreter, "hex", word_hex,
            "Convert a number into a hex string.",
            "number -- hex_string");
    }


}
