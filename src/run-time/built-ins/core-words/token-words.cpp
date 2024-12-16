
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void word_token_is_string(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            if (value.is_token())
            {
                auto token = value.as_token(interpreter);
                interpreter->push(token.type == Token::Type::string);
            }
            else
            {
                throw_error(interpreter, "Value not a token.");
            }
        }


        void word_token_is_word(InterpreterPtr& interpreter)
        {
            auto token = interpreter->pop_as_token();

            interpreter->push(token.type == Token::Type::word);
        }


        void word_token_is_number(InterpreterPtr& interpreter)
        {
            auto token = interpreter->pop_as_token();

            interpreter->push(token.type == Token::Type::number);
        }


        void word_token_text(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop_as_token();

            interpreter->push(value.text);
        }


        void word_token_integer(InterpreterPtr& interpreter)
        {
            auto token = interpreter->pop_as_token();

            //if (token.type == Token::Type::number)
            //{
            //    interpreter->push(token.);
            //}
            //else
            //{
            //    throw_error(interpreter, "Token is not a number.");
            //}

            throw_error(interpreter, "Not implemented.");
        }


        void word_token_float(InterpreterPtr& interpreter)
        {
            auto token = interpreter->pop_as_token();

            throw_error(interpreter, "Not implemented.");
        }


    }


    void register_token_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "token.is_string?", word_token_is_string,
            "Does the token represent a string value?",
            "token -- bool");

        ADD_NATIVE_WORD(interpreter, "token.is_word?", word_token_is_word,
            "Does the token represent a word value?",
            "token -- bool");

        ADD_NATIVE_WORD(interpreter, "token.is_number?", word_token_is_number,
            "Does the token represent a numeric value?",
            "token -- bool");

        ADD_NATIVE_WORD(interpreter, "token.text@", word_token_text,
            "Get the text value from a token.",
            "token -- string");

        ADD_NATIVE_WORD(interpreter, "token.integer@", word_token_integer,
            "Get the integer value from a token.",
            "token -- int");

        ADD_NATIVE_WORD(interpreter, "token.float@", word_token_float,
            "Get the floating point value from a token.",
            "token -- float");
    }


}
