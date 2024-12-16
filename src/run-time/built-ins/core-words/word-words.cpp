
#include "sorth.h"
#include "word-info-struct.h"



namespace sorth::internal
{


    namespace
    {


        void word_word(InterpreterPtr& interpreter)
        {
            auto& current_token = interpreter->constructor().current_token;
            auto& input_tokens = interpreter->constructor().input_tokens;

            throw_error_if(current_token >= input_tokens.size(), interpreter,
                        "word trying to read past the end of the token list.");

            interpreter->push(input_tokens[++current_token]);
        }


        void word_get_word_table(InterpreterPtr& interpreter)
        {
            auto dictionary = interpreter->get_dictionary().get_merged_dictionary();
            HashTablePtr new_table = std::make_shared<HashTable>();

            for (auto word : dictionary)
            {
                auto key = word.first;
                auto value = make_word_info_instance(interpreter, key, word.second);

                new_table->insert(key, value);
            }

            interpreter->push(new_table);
        }


        void word_word_index(InterpreterPtr& interpreter)
        {
            auto& current_token = interpreter->constructor().current_token;
            auto& input_tokens = interpreter->constructor().input_tokens;

            ++current_token;
            auto name = input_tokens[current_token].text;

            auto [found, word] = interpreter->find_word(name);

            if (found)
            {
                interpreter->constructor().stack.top().code.push_back({
                        .id = Instruction::Id::push_constant_value,
                        .value = (int64_t)word.handler_index
                    });
            }
            else
            {
                interpreter->constructor().stack.top().code.push_back({
                        .id = Instruction::Id::word_index,
                        .value = name
                    });
            }
        }


        void word_execute(InterpreterPtr& interpreter)
        {
            auto word_value = interpreter->pop();

            if (word_value.is_numeric())
            {
                auto index = static_cast<size_t>(word_value.as_integer(interpreter));
                auto& handler = interpreter->get_handler_info(index);

                handler.function(interpreter);
            }
            else if (word_value.is_string())
            {
                auto name = word_value.as_string(interpreter);

                interpreter->execute_word(name);
            }
            else
            {
                throw_error(interpreter, "Unexpected value type for execute.");
            }
        }


        void word_is_defined(InterpreterPtr& interpreter)
        {
            auto& current_token = interpreter->constructor().current_token;
            auto& input_tokens = interpreter->constructor().input_tokens;

            ++current_token;
            auto name = input_tokens[current_token].text;

            interpreter->constructor().stack.top().code.push_back({
                    .id = Instruction::Id::word_exists,
                    .value = name
                });
        }


        void word_is_defined_im(InterpreterPtr& interpreter)
        {
            auto& current_token = interpreter->constructor().current_token;
            auto& input_tokens = interpreter->constructor().input_tokens;

            ++current_token;
            auto name = input_tokens[current_token].text;

            auto found = std::get<0>(interpreter->find_word(name));

            interpreter->push(found);
        }


        void word_is_undefined_im(InterpreterPtr& interpreter)
        {
            word_is_defined_im(interpreter);
            auto found = interpreter->pop_as_bool();

            interpreter->push(!found);
        }


    }


    void register_word_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "word", word_word,
            "Get the next word in the token stream.",
            " -- next_word");

        ADD_NATIVE_WORD(interpreter, "words.get{}", word_get_word_table,
            "Get a copy of the word table as it exists at time of calling.",
            " -- all_defined_words");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "`", word_word_index,
            "Get the index of the next word.",
            " -- index");

        ADD_NATIVE_WORD(interpreter, "execute", word_execute,
            "Execute a word name or index.",
            "word_name_or_index -- ???");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "defined?", word_is_defined,
            "Is the given word defined?",
            " -- bool");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "[defined?]", word_is_defined_im,
            "Evaluate at compile time, is the given word defined?",
            " -- bool");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "[undefined?]", word_is_undefined_im,
            "Evaluate at compile time, is the given word not defined?",
            " -- bool");
    }


}
