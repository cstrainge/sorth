
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void word_hash_table_new(InterpreterPtr& interpreter)
        {
            auto table = std::make_shared<HashTable>();
            interpreter->push(table);
        }


        void word_hash_table_insert(InterpreterPtr& interpreter)
        {
            auto table = interpreter->pop_as_hash_table();
            auto key = interpreter->pop();
            auto value = interpreter->pop();

            table->insert(key, value);
        }


        void word_hash_table_find(InterpreterPtr& interpreter)
        {
            auto table = interpreter->pop_as_hash_table();
            auto key = interpreter->pop();

            auto [ found, value ] = table->get(key);

            if (!found)
            {
                std::stringstream stream;

                stream << "Value, " << key << ", does not exist in the table.";

                throw_error(interpreter, stream.str());
            }

            interpreter->push(value);
        }


        void word_hash_table_exists(InterpreterPtr& interpreter)
        {
            auto table = interpreter->pop_as_hash_table();
            auto key = interpreter->pop();

            auto [ found, value ] = table->get(key);

            interpreter->push(found);
        }


        void word_hash_plus(InterpreterPtr& interpreter)
        {
            auto hash_src = interpreter->pop_as_hash_table();
            auto hash_dest = interpreter->pop_as_hash_table();

            for (auto entry : hash_src->get_items())
            {
                auto key = entry.first;
                auto value = entry.second;

                hash_dest->insert(key.deep_copy(), value.deep_copy());
            }

            interpreter->push(hash_dest);
        }


        void word_hash_compare(InterpreterPtr& interpreter)
        {
            auto hash_a = interpreter->pop_as_hash_table();
            auto hash_b = interpreter->pop_as_hash_table();

            interpreter->push(hash_a == hash_b);
        }


        void word_hash_table_size(InterpreterPtr& interpreter)
        {
            auto hash = interpreter->pop_as_hash_table();

            interpreter->push(hash->size());
        }


        void word_hash_table_iterate(InterpreterPtr& interpreter)
        {
            auto table = interpreter->pop_as_hash_table();
            auto word_index = interpreter->pop_as_size();

            auto& handler = interpreter->get_handler_info(word_index);

            for (const auto& item : table->get_items())
            {
                interpreter->push(item.first);
                interpreter->push(item.second);

                handler.function(interpreter);
            }
        }


    }


    void register_hash_table_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "{}.new", word_hash_table_new,
            "Create a new hash table.",
            " -- new_hash_table");

        ADD_NATIVE_WORD(interpreter, "{}!", word_hash_table_insert,
            "Write a value to a given key in the table.",
            "value key table -- ");

        ADD_NATIVE_WORD(interpreter, "{}@", word_hash_table_find,
            "Read a value from a given key in the table.",
            "key table -- value");

        ADD_NATIVE_WORD(interpreter, "{}?", word_hash_table_exists,
            "Check if a given key exists in the table.",
            "key table -- bool");

        ADD_NATIVE_WORD(interpreter, "{}.+", word_hash_plus,
            "Take two hashes and deep copy the contents from the second into the first.",
            "dest source -- dest");

        ADD_NATIVE_WORD(interpreter, "{}.=", word_hash_compare,
            "Take two hashes and compare their contents.",
            "a b -- was-match");

        ADD_NATIVE_WORD(interpreter, "{}.size@", word_hash_table_size,
            "Get the size of the hash table.",
            "table -- size");

        ADD_NATIVE_WORD(interpreter, "{}.iterate", word_hash_table_iterate,
            "Iterate through a hash table and call a word for each item.",
            "word_index hash_table -- ");
    }


}
