
#include "sorth.h"



namespace sorth::internal
{


    // Create a new compile context and take ownership of the given tokens.
    CompileContext::CompileContext(InterpreterPtr& interpreter, TokenList&& tokens) noexcept
    : interpreter_wptr(interpreter),
      input_tokens(std::move(tokens))
    {
        // Always start with one construction on the stack.
        new_construction();
    }


    // Compile tokens until one of the given word tokens is found, or it will throw an error if the
    // end of the token stream is reached first.
    std::string CompileContext::compile_until_words(const std::vector<std::string>& word_list)
    {
        // Is the word name one of the words we are looking for?  If it is, return a true and the
        // name of the word we found.
        auto is_one_of_words = [&](const Token& match) -> std::tuple<bool, std::string>
            {
                // Make sure that the given token is allowed to be a word name.
                if (match.type != Token::Type::string)
                {
                    // Grab the interpreter.
                    InterpreterPtr interpreter = interpreter_wptr.lock();

                    // Ok, it's a valid word name, so convert the token to a string.
                    auto match_string = match.text;

                    // Check to see if we have a match with our word list...
                    for (const auto& word : word_list)
                    {
                        if (word == match_string)
                        {
                            // We found a match, so return it.
                            return { true, word };
                        }
                    }
                }

                // We didn't find a match, so return false and an empty string.
                return { false, "" };
            };


        // Keep track of the location of the first token in the list, so we can throw an error
        // with the location of the start of the search if we don't find a match.  That should
        // be more useful than the location of the last token we checked.
        std::optional<Location> start_location;

        // Loop through the tokens in the list until we find a match or the end of the list is
        // reached.
        while (current_token < (input_tokens.size() - 1))
        {
            // Grab the next token in the list.  If we haven't recorded a starting location, do
            // so now.
            const auto& token = get_next_token();

            if (!start_location)
            {
                start_location = token.location;
            }

            // Is this one of the word token's we're looking for?
            auto [ found, word ] = is_one_of_words(token);

            if (found)
            {
                // It is, so we're done here.
                return word;
            }

            // We haven't found our word, so compile the token as normal.
            compile_token(token);
        }

        // If we got here, then we didn't find a match, so throw an error.
        std::string message;

        // Generate a message to let the user know what word we were looking for and where the
        // search started so they can find it in their source.
        if (word_list.size() == 1)
        {
            message = "Missing word " + word_list[0] + " in source.";
        }
        else
        {
            std::stringstream stream;

            stream << "Missing matching word, expected one of [ ";

            for (auto word : word_list)
            {
                stream << word << " ";
            }

            stream << "]";

            message = stream.str();
        }

        // Throw the message to the caller.  Again, we don't use the interpreter's location
        // because it's not as useful as the start location of the search.
        //
        // TODO: Include the call stack as well?
        throw_error(start_location.value(), message);
    }


    void CompileContext::compile_token(const Token& token)
    {
        // Get a shared pointer to the interpreter, now that we need to access it.
        InterpreterPtr interpreter = interpreter_wptr.lock();

        // In Forth anything can be a word, so first we see if it's defined in the dictionary.  If
        // it is, we either compile or execute the word depending on if it's an immediate.
        auto [ found, word ] = token.type != Token::Type::string
                               ? interpreter->find_word(token.text)
                               : std::tuple<bool, Word>{ false, {} };

        if (found)
        {
            if (word.is_immediate)
            {
                interpreter->execute_word(token.location, word);
            }
            else
            {
                insert_instruction(
                    {
                        .id = Instruction::Id::execute,
                        .value = static_cast<int64_t>(word.handler_index),
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

                        insert_instruction(
                            {
                                .id = Instruction::Id::push_constant_value,
                                .value = value
                            });
                    }
                    break;

                case Token::Type::string:
                    insert_instruction(
                        {
                            .id = Instruction::Id::push_constant_value,
                            .value = token.text
                        });
                    break;

                case Token::Type::word:
                    {
                        // This word wasn't found, so leave it for resolution at run time.
                        insert_instruction(
                            {
                                .id = Instruction::Id::execute,
                                .value = token.text,
                                .location = token.location
                            });
                    }
                    break;
            }
        }
    }


    void CompileContext::compile_token_list()
    {
        // Whip through the tokens in the list.  Each token will be compiled into the current
        // construction.
        //
        // During the execution of compile_token all kinds of things can happen.  The construction
        // stack can grow or shrink, the current construction can change, and the current token can
        // change.  This is why the index is a member variable of the class and can be incremented
        // by the next_token function.
        //
        // User words can also be executed which can change the state of the interpreter.  Script
        // compilation is a very dynamic process and interwoven with the execution of the script.
        for (current_token = 0; current_token < input_tokens.size(); ++current_token)
        {
            const Token& token = input_tokens[current_token];
            compile_token(token);
        }
    }


    // Get the next token in the token stream, advancing the current index.  Or we throw an error if
    // we're at the end of the token list.
    const Token& CompileContext::get_next_token()
    {
        // Make sure we're not at the end of the token list.
        if (current_token >= input_tokens.size())
        {
            // Get a shared pointer to the interpreter now that we need to access it.
            const InterpreterPtr interpreter = interpreter_wptr.lock();

            throw_error(interpreter, "Attempted to read past the end of the token list.");
        }

        // Get the next token and increment the index.
        ++current_token;
        const auto& next_token = input_tokens[current_token];

        return next_token;
    }


    // Create a new code construction and push it onto the stack.
    void CompileContext::new_construction() noexcept
    {
        stack.push({});
    }


    void CompileContext::new_construction(const std::string& name, const Location& location) noexcept
    {
        stack.push(
            {
                .name = name,
                .location = location
            });
    }


    // Create a new code construction with the given bytecode and push it onto the stack.
    void CompileContext::new_construction(ByteCode&& code) noexcept
    {
        Construction construction;

        construction.code = std::move(code);
        stack.push(construction);
    }


    // Drop the current top construction from the stack, returning it to the caller.
    Construction CompileContext::drop_construction()
    {
        if (stack.empty())
        {
            throw_error("Attempted to drop a construction from an empty stack.");
        }

        auto construction = stack.top();
        stack.pop();

        return construction;
    }


    // Merge the top construction into the one below it.  If there aren't at least two constructions
    // on the stack, throw an error.
    void CompileContext::merge_construction() noexcept
    {
        if (stack.size() < 2)
        {
            throw_error("Attempted to merge the top construction into the one below it, but there "
                        "are not enough constructions on the stack.");
        }

        Construction other = drop_construction();
        auto& code = construction().code;

        code.insert(code.end(), other.code.begin(), other.code.end());
    }


    // Get the current top construction on the stack.
    Construction& CompileContext::construction()
    {
        if (stack.empty())
        {
            // Get a shared pointer to the interpreter now that we need to access it.
            const InterpreterPtr interpreter = interpreter_wptr.lock();

            // This shouldn't happen as there should always be at least one construction on the
            // stack to represent the script's top level code.
            throw_error(interpreter, "Attempted to access the top construction on an empty stack.");
        }

        // Return what we got.
        return stack.top();
    }


    // Set the insertion point for new code.
    void CompileContext::set_insertion(CodeInsertionPoint point) noexcept
    {
        insertion_point = point;
    }


    // Insert a new instruction into the current construction at the current insertion point.
    void CompileContext::insert_instruction(const Instruction& instruction) noexcept
    {
        // Insert the instruction at the current insertion point.
        if (insertion_point == CodeInsertionPoint::end)
        {
            construction().code.push_back(instruction);
        }
        else
        {
            construction().code.insert(construction().code.begin(), instruction);
        }
    }


}
