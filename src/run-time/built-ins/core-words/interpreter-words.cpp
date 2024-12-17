
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void word_reset(InterpreterPtr& interpreter)
        {
            interpreter->release_context();
            interpreter->clear_stack();

            reset_ffi();

            interpreter->mark_context();
        }


        void word_include(InterpreterPtr& interpreter)
        {
            auto path = interpreter->pop_as_string();
            interpreter->process_source(interpreter->find_file(path));
        }


        void word_include_im(InterpreterPtr& interpreter)
        {
            auto& token = interpreter->compile_context().get_next_token();
            interpreter->process_source(interpreter->find_file(token.text));
        }


        void word_if_im(InterpreterPtr& interpreter)
        {
            auto is_one_of = [](std::string& match, std::vector<std::string>& words) -> bool
                {
                    for (auto& word : words)
                    {
                        if (match == word)
                        {
                            return true;
                        }
                    }

                    return false;
                };

            auto skip_until = [&](std::vector<std::string> words) -> std::string
                {
                    auto token = interpreter->compile_context().get_next_token();

                    while (!is_one_of(token.text, words))
                    {
                        token = interpreter->compile_context().get_next_token();
                    }

                    return token.text;
                };


            auto result = interpreter->pop_as_bool();

            if (result)
            {
                auto found = interpreter->compile_context().compile_until_words(
                    {
                        "[else]",
                        "[then]"
                    });

                if (found == "[else]")
                {
                    skip_until({ "[then]" });
                }
            }
            else
            {
                auto found = skip_until({ "[else]", "[then]" });

                if (found == "[else]")
                {
                    interpreter->compile_context().compile_until_words({ "[then]" });
                }
            }
        }


        void word_thread_show(InterpreterPtr& interpreter)
        {
            auto list = interpreter->sub_threads();

            for (auto& item : list)
            {
                auto info = interpreter->get_handler_info(item.word.handler_index);

                std::cout << "Thread " << item.word_thread->get_id()
                        << " word " << info.name << ", (" << item.word.handler_index << "), "

                        << "I/O queues (" << item.inputs.depth() << "/"
                                            << item.outputs.depth() << ")"
                        << (item.thead_deleted ? " has completed." : " is running.")
                        << std::endl;
            }
        }


        void word_thread_new(InterpreterPtr& interpreter)
        {
            auto name = interpreter->pop_as_string();
            auto [ found, word ] = interpreter->find_word(name);

            if (!found)
            {
                throw_error(interpreter, "Could not start thread, word " + name + " not found.");
            }

            auto id = interpreter->execute_word_threaded(word);
            interpreter->push(id);
        }


        void word_thread_push_to(InterpreterPtr& interpreter)
        {
            auto id = interpreter->pop_as_thread_id();
            auto value = interpreter->pop();

            interpreter->thread_push_input(id, value);
        }


        void word_thread_pop_from(InterpreterPtr& interpreter)
        {
            auto id = interpreter->pop_as_thread_id();
            auto value = interpreter->thread_pop_output(id);

            interpreter->push(value);
        }


        void word_thread_push(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();
            auto id = std::this_thread::get_id();

            interpreter->thread_push_output(id, value);
        }


        void word_thread_pop(InterpreterPtr& interpreter)
        {
            auto id = std::this_thread::get_id();

            interpreter->push(interpreter->thread_pop_input(id));
        }


        void word_throw(InterpreterPtr& interpreter)
        {
            throw_error(interpreter, interpreter->pop_as_string());
        }


        void word_unique_str(InterpreterPtr& interpreter)
        {
            static std::atomic<int64_t> index = 0;

            std::stringstream stream;
            auto current = index.fetch_add(1, std::memory_order_relaxed);

            stream << "unique-" << std::setw(4) << std::setfill('0') << std::hex << current;

            interpreter->push(stream.str());
        }


        void word_exit_success(InterpreterPtr& interpreter)
        {
            interpreter->push((int64_t)EXIT_SUCCESS);
        }


        void word_exit_failure(InterpreterPtr& interpreter)
        {
            interpreter->push((int64_t)EXIT_FAILURE);
        }


        void word_set_exit_code(InterpreterPtr& interpreter)
        {
            auto code = interpreter->pop_as_integer();

            interpreter->set_exit_code(code);
        }


        void word_none(InterpreterPtr& interpreter)
        {
            interpreter->push(None());
        }


        void word_true(InterpreterPtr& interpreter)
        {
            interpreter->push(true);
        }


        void word_false(InterpreterPtr& interpreter)
        {
            interpreter->push(false);
        }


        void word_print_stack(InterpreterPtr& interpreter)
        {
            interpreter->print_stack(std::cout);
        }


        void word_print_dictionary(InterpreterPtr& interpreter)
        {
            interpreter->print_dictionary(std::cout);
        }


        void word_get_sorth_version(InterpreterPtr& interpreter)
        {
            interpreter->push(std::string(SORTH_VERSION));
        }


        void word_get_compiler_version(InterpreterPtr& interpreter)
        {
            interpreter->push(std::string(SORTH_COMPILER));
        }


        void word_show_bytecode(InterpreterPtr& interpreter)
        {
            interpreter->showing_bytecode() = interpreter->pop_as_bool();
        }


        void word_show_run_code(InterpreterPtr& interpreter)
        {
            interpreter->showing_run_code() = interpreter->pop_as_bool();
        }


        void word_show_word_bytecode(InterpreterPtr& interpreter)
        {
            std::string name;
            auto value = interpreter->pop();

            if (value.is_string())
            {
                name = value.as_string(interpreter);
            }
            else if (value.is_numeric())
            {
                auto index = value.as_integer(interpreter);
                auto info = interpreter->get_handler_info(index);

                name = info.name;
            }
            else
            {
                throw_error(interpreter->get_current_location(), "Expected a word name or index.");
            }

            auto [ found, word ] = interpreter->find_word(name);

            if (!found)
            {
                std::cerr << "Word " << name << " has not been defined." << std::endl;
                return;
            }

            auto& handler_info = interpreter->get_handler_info(word.handler_index);
            auto optional_code = handler_info.function.get_byte_code();

            if (optional_code.has_value())
            {
                pretty_print_bytecode(interpreter, optional_code.value(), std::cout);
            }
            else
            {
                std::cerr << "Word " << name << " doesn't contain byte-code." << std::endl;
            }
        }


        void word_show_ir(InterpreterPtr& interpreter)
        {
            std::string name;
            auto value = interpreter->pop();

            if (value.is_string())
            {
                name = value.as_string(interpreter);
            }
            else if (value.is_numeric())
            {
                auto index = value.as_integer(interpreter);
                auto info = interpreter->get_handler_info(index);

                name = info.name;
            }
            else
            {
                throw_error(interpreter->get_current_location(), "Expected a word name or index.");
            }

            auto [ found, word ] = interpreter->find_word(name);

            if (!found)
            {
                std::cerr << "Word " << name << " has not been defined." << std::endl;
                return;
            }

            auto& handler_info = interpreter->get_handler_info(word.handler_index);
            auto optional_ir = handler_info.function.get_ir();

            if (optional_ir.has_value())
            {
                std::cout << optional_ir.value();
            }
            else
            {
                std::cerr << "Word " << name << " doesn't contain IR." << std::endl;
            }
        }


        void word_show_machine_code(InterpreterPtr& interpreter)
        {
            std::string name;
            auto value = interpreter->pop();

            if (value.is_string())
            {
                name = value.as_string(interpreter);
            }
            else if (value.is_numeric())
            {
                auto index = value.as_integer(interpreter);
                auto info = interpreter->get_handler_info(index);

                name = info.name;
            }
            else
            {
                throw_error(interpreter->get_current_location(), "Expected a word name or index.");
            }

            auto [ found, word ] = interpreter->find_word(name);

            if (!found)
            {
                std::cerr << "Word " << name << " has not been defined." << std::endl;
                return;
            }

            auto& handler_info = interpreter->get_handler_info(word.handler_index);
            auto optional_asm_code = handler_info.function.get_asm_code();

            if (optional_asm_code.has_value())
            {
                std::cout << optional_asm_code.value();
            }
            else
            {
                std::cerr << "Word " << name << " doesn't contain generated machine code." << std::endl;
            }
        }


        void word_sorth_execution_mode(InterpreterPtr& interpreter)
        {
            auto mode = interpreter->get_execution_mode();

            switch (mode)
            {
                case ExecutionMode::byte_code:
                    interpreter->push("byte-code");
                    break;

                case ExecutionMode::jit:
                    interpreter->push("jit");
                    break;

                default:
                    interpreter->push("unknown");
                    break;
            }
        }


    }


    void register_interpreter_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "reset", word_reset,
            "Reset the interpreter to it's default state.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "include", word_include,
            "Include and execute another source file.",
            "source_path -- ");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "[include]", word_include_im,
            "Include and execute another source file.",
            "[include] path/to/file.f");

        ADD_NATIVE_IMMEDIATE_WORD(interpreter, "[if]", word_if_im,
            "Evaluate an if at compile time.  Only the code on successful branch is compiled.",
            "[if] <code> [else] <code> [then]");

        ADD_NATIVE_WORD(interpreter, ".t", word_thread_show,
            "Print out the list of interpreter threads.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "thread.new", word_thread_new,
            "Create a new thread and run the specified word and return the new thread id.",
            "word-index -- thread-id");

        ADD_NATIVE_WORD(interpreter, "thread.push-to", word_thread_push_to,
            "Push the top value to another thread's input stack.",
            "value thread-id -- ");

        ADD_NATIVE_WORD(interpreter, "thread.pop-from", word_thread_pop_from,
            "Pop a value off of the threads input queue, block if there's nothing available.",
            "thread-id -- input-value");

        ADD_NATIVE_WORD(interpreter, "thread.push", word_thread_push,
            "Push the top value onto the thread's output queue.",
            "output-value -- ");

        ADD_NATIVE_WORD(interpreter, "thread.pop", word_thread_pop,
            "Pop from another thread's output stack and push onto the local data stack.",
            " -- value");

        ADD_NATIVE_WORD(interpreter, "throw", word_throw,
            "Throw an exception with the given message.",
            "message -- ");

        ADD_NATIVE_WORD(interpreter, "unique_str", word_unique_str,
            "Generate a unique string and push it onto the data stack.",
            " -- string");

        ADD_NATIVE_WORD(interpreter, "exit_success", word_exit_success,
            "Constant value for a process success exit code.",
            " -- success");

        ADD_NATIVE_WORD(interpreter, "exit_failure", word_exit_failure,
            "Constant value for a process fail exit code.",
            " -- failure");

        ADD_NATIVE_WORD(interpreter, "sorth.exit-code!", word_set_exit_code,
            "Set the exit code for the interpreter.",
            "exit-code -- ");

        ADD_NATIVE_WORD(interpreter, "none", word_none,
            "Push the value none onto the data stack.",
            " -- none");

        ADD_NATIVE_WORD(interpreter, "true", word_true,
            "Push the value true onto the data stack.",
            " -- true");

        ADD_NATIVE_WORD(interpreter, "false", word_false,
            "Push the value false onto the data stack.",
            " -- false");

        ADD_NATIVE_WORD(interpreter, ".s", word_print_stack,
            "Print out the data stack without changing it.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, ".w", word_print_dictionary,
            "Print out the current word dictionary.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "sorth.version", word_get_sorth_version,
            "Get the current version of the interpreter.",
            " -- version_string");

        ADD_NATIVE_WORD(interpreter, "sorth.compiler", word_get_compiler_version,
            "Get name and version of the compiler that built the interpreter.",
            " -- compiler_info");

        ADD_NATIVE_WORD(interpreter, "sorth.show-bytecode", word_show_bytecode,
            "Show detailed information about a word.",
            "word -- ");

        ADD_NATIVE_WORD(interpreter, "sorth.show-ir", word_show_ir,
            "Show the generated LLVM IR for the word.",
            "word -- ");

        ADD_NATIVE_WORD(interpreter, "sorth.show-asm", word_show_machine_code,
            "Show the generated machine code for the word.",
            "word -- ");

        ADD_NATIVE_WORD(interpreter, "sorth.execution-mode", word_sorth_execution_mode,
            "Get the current execution mode of the interpreter, either 'jit' or 'byte-code'.",
            " -- mode");
    }


}
