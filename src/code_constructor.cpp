
#include "sorth.h"



namespace sorth::internal
{


    void CodeConstructor::compile_token(const Token& token)
    {
        // In Forth anything can be a word, so first we see if it's defined in the dictionary.  If
        // it is, we either compile or execute the word depending on if it's an immediate.
        auto [found, word] = token.type != Token::Type::string
                             ? interpreter->find_word(token.text)
                             : std::tuple<bool, Word>{ false, {} };

        if (found)
        {
            if (word.is_immediate)
            {
                Word the_word = word;
                interpreter->execute_word(token.location, the_word);
            }
            else
            {
                stack.top().code.push_back({
                        .id = OperationCode::Id::execute,
                        .value = (int64_t)word.handler_index,
                        .location = token.location
                    });
            }
        }
        else
        {
            // We didn't find a word, so try to process the token as the tokenizer found it.
            switch (token.type)
            {
                case Token::Type::number:
                    {
                        Value value;

                        if (token.text.find('.') != std::string::npos)
                        {
                            value = std::stod(token.text);
                        }
                        else if ((token.text[0] == '0') && (token.text[1] == 'x'))
                        {
                            value = (int64_t)std::stoll(token.text, nullptr, 16);
                        }
                        else if ((token.text[0] == '0') && (token.text[1] == 'b'))
                        {
                            value = (int64_t)std::stoll(token.text.substr(2), nullptr, 2);
                        }
                        else
                        {
                            value = (int64_t)std::stoll(token.text);
                        }

                        stack.top().code.push_back({
                                .id = OperationCode::Id::push_constant_value,
                                .value = value
                            });
                    }
                    break;

                case Token::Type::string:
                    stack.top().code.push_back({
                            .id = OperationCode::Id::push_constant_value,
                            .value = token.text
                        });
                    break;

                case Token::Type::word:
                    {
                        // This word wasn't found, so leave it for resolution at run time.
                        stack.top().code.push_back({
                                .id = OperationCode::Id::execute,
                                .value = token.text,
                                .location = token.location
                            });
                    }
                    break;
            }
        }
    }


    void CodeConstructor::compile_token_list()
    {
        for (current_token = 0; current_token < input_tokens.size(); ++current_token)
        {
            const Token& token = input_tokens[current_token];
            compile_token(token);
        }

        /*if (is_showing_bytecode)
        {
            std::cerr << stack.top().code;
        }*/
    }


}
