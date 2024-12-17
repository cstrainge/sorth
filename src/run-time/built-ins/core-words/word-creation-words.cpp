
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        // Class for handling script defined words.  Every user defined word is an instance of this
        // class.
        class ScriptWord
        {
            private:
                struct ContextManager
                {
                    InterpreterPtr interpreter;

                    ContextManager(InterpreterPtr& new_interpreter)
                    : interpreter(new_interpreter)
                    {
                        interpreter->mark_context();
                    }

                    ~ContextManager()
                    {
                        if (interpreter)
                        {
                            interpreter->release_context();
                        }
                    }
                };

            private:
                std::string name;
                bool is_context_managed;
                ByteCode code;

            public:
                ScriptWord(const std::string& new_name,
                           const ByteCode& new_code,
                           const Location& new_location,
                           const bool new_is_context_managed)
                : name(new_name),
                  code(new_code),
                  is_context_managed(new_is_context_managed)
                {
                }

            public:
                void operator ()(InterpreterPtr& interpreter)
                {
                    if (is_context_managed)
                    {
                        ContextManager manager(interpreter);
                        interpreter->execute_code(name, code);
                    }
                    else
                    {
                        interpreter->execute_code(name, code);
                    }
                }

            public:
                const ByteCode& get_code() const
                {
                    return code;
                }
        };


        void word_start_word(InterpreterPtr& interpreter)
        {
            auto& current_token = interpreter->compile_context().current_token;
            auto& input_tokens = interpreter->compile_context().input_tokens;

            ++current_token;
            auto& name = input_tokens[current_token].text;
            auto& location = input_tokens[current_token].location;

            interpreter->compile_context().stack.push({
                    .is_immediate = false,
                    .is_hidden = false,
                    .is_context_managed = true,
                    .name = name,
                    .description = "",
                    .location = location
                });
        }


        void word_end_word(InterpreterPtr& interpreter)
        {
            // Pop the current construction off of the stack.
            auto construction = interpreter->compile_context().stack.top();
            interpreter->compile_context().stack.pop();

            // The word handler we will be registering with the interpreter.  It will be either a
            // byte-code handler or a JITed handler based on support and the mode we're in.
            WordFunction handler;

            // The word is considered scripted, as it is word written in Forth.
            const bool is_scripted = true;


            // Check to see if we are built for JIT compilation or not, and if so, check to see if we
            // are in JIT mode or not.
            #if !defined(SORTH_JIT_DISABLED)
            if (interpreter->get_execution_mode() == ExecutionMode::jit)
            {
                // If the word is not immediate, then we can cache the construction to be JIT compiled
                // when the whole script is compiled.
                if (!construction.is_immediate)
                {
                    // Add the construction to the JIT cache.
                    interpreter->compile_context().word_jit_cache[construction.name] = construction;

                    // Create a script word handler for the word for now.  This will be replaced with
                    // the JITed handler when the script is compiled.
                    //
                    // However in the mean time, immediate words may need these words, byte-code or not.
                    auto script_word = ScriptWord(construction.name,
                                                  construction.code,
                                                  construction.location,
                                                  construction.is_context_managed);
                    handler = script_word;
                    handler.set_byte_code(std::move(construction.code));
                }
                else
                {
                    // Otherwise we need to JIT compile the word now in it's own module.
                    handler = jit_immediate_word(interpreter, construction);
                    handler.set_byte_code(std::move(construction.code));
                }
            }
            else
            #endif
            {
                // We are byte-code interpreting, so we need to create a script word handler.  In this
                // case it doesn't matter if the word is immediate or not.
                auto script_word = ScriptWord(construction.name,
                                            construction.code,
                                            construction.location,
                                            construction.is_context_managed);

                // Pretty print the bytecode if we are in debug mode.
                if (interpreter->showing_bytecode())
                {
                    std::cout << "--------[" << construction.name << "]-------------" << std::endl;

                    pretty_print_bytecode(interpreter, construction.code, std::cout);
                }

                handler = script_word;
                handler.set_byte_code(std::move(construction.code));
            }

            // Register the word either byte-code or JITed with the interpreter.
            interpreter->add_word(construction.name,
                                handler,
                                construction.location,
                                construction.is_immediate,
                                construction.is_hidden,
                                is_scripted,
                                construction.description,
                                construction.signature);
        }


        void word_immediate(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().stack.top().is_immediate = true;
        }


        void word_hidden(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().stack.top().is_hidden = true;
        }


        void word_contextless(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().stack.top().is_context_managed = false;
        }


        void word_description(InterpreterPtr& interpreter)
        {
            auto& current_token = interpreter->compile_context().current_token;
            auto& input_tokens = interpreter->compile_context().input_tokens;

            ++current_token;

            throw_error_if(current_token >= input_tokens.size(), interpreter,
                "Unexpected end to token stream.");

            auto& token = input_tokens[current_token];

            throw_error_if(token.type != Token::Type::string,
                        interpreter,
                        "Expected the description to be a string.");

            interpreter->compile_context().stack.top().description = token.text;
        }


        void word_signature(InterpreterPtr& interpreter)
        {
            auto& current_token = interpreter->compile_context().current_token;
            auto& input_tokens = interpreter->compile_context().input_tokens;

            ++current_token;

            throw_error_if(current_token >= input_tokens.size(), interpreter,
                        "Unexpected end to token stream.");

            auto& token = input_tokens[current_token];

            throw_error_if(token.type != Token::Type::string,
                        interpreter,
                        "Expected the signature to be a string.");

            interpreter->compile_context().stack.top().signature = token.text;
        }


    }


    void register_word_creation_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_IMMEDIATE_WORD(interpreter, ":", word_start_word,
            "The start of a new word definition.",
            " -- ");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, ";", word_end_word,
            "The end of a new word definition.",
            " -- ");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "immediate", word_immediate,
            "Mark the current word being built as immediate.",
            " -- ");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "hidden", word_hidden,
            "Mark the current word being built as hidden.",
            " -- ");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "contextless", word_contextless,
            "Disable automatic context management for the word.",
            " -- ");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "description:", word_description,
            "Give a new word it's description.",
            " -- ");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "signature:", word_signature,
            "Describe a new word's stack signature.",
            " -- ");
    }


}
