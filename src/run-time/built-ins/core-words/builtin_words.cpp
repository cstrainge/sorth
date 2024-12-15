
#include <stdlib.h>
#include <cstdlib>
#include <iomanip>

#include "sorth.h"



namespace sorth
{


    using namespace internal;


    namespace
    {


        // When user code registers an at_exit handler, we need to keep track of the interpreter
        // the handler lives in as well as the index or name of that handler to execute.
        InterpreterPtr at_exit_interpreter;
        Value at_exit_value;


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


        // Helpers for some of the words.
        void throw_if_out_of_bounds(InterpreterPtr& interpreter, int64_t index, int64_t size,
                                    const std::string& type)
        {
            if ((index >= size) || (index < 0))
            {
                std::stringstream stream;

                stream << type << " index, " << index << ", is out of bounds of the size " << size
                       << ".";

                throw_error(interpreter, stream.str());
            }
        }


        void check_buffer_index(InterpreterPtr& interpreter,
                                const ByteBufferPtr& buffer, int64_t byte_size)
        {
            if ((buffer->position() + byte_size) > buffer->size())
            {
                std::stringstream stream;

                stream << "Writing a value of size " << byte_size << " at a position of "
                       << buffer->position() << " would exceed the buffer size, "
                       << buffer->size() << ".";

                throw_error(interpreter, stream.str());
            }
        }


        void math_op(InterpreterPtr& interpreter,
                     std::function<double(double, double)> dop,
                     std::function<int64_t(int64_t, int64_t)> iop)
        {
            Value b = interpreter->pop();
            Value a = interpreter->pop();
            Value result;

            if (either_is<double>(a, b))
            {
                result = dop(as_numeric<double>(interpreter, a),
                             as_numeric<double>(interpreter, b));
            }
            else
            {
                result = iop(as_numeric<int64_t>(interpreter, a),
                             as_numeric<int64_t>(interpreter, b));
            }

            interpreter->push(result);
        }


        void logic_op(InterpreterPtr& interpreter, std::function<bool(bool, bool)> op)
        {
            auto b = interpreter->pop();
            auto a = interpreter->pop();
            auto result = op(as_numeric<bool>(interpreter, a),
                             as_numeric<bool>(interpreter, b));

            interpreter->push(result);
        }


        void logic_bit_op(InterpreterPtr& interpreter, std::function<int64_t(int64_t, int64_t)> op)
        {
            auto b = interpreter->pop();
            auto a = interpreter->pop();
            auto result = op(as_numeric<int64_t>(interpreter, a),
                             as_numeric<int64_t>(interpreter, b));

            interpreter->push(result);
        }


        void string_or_numeric_op(InterpreterPtr& interpreter,
                                  std::function<void(double, double)> dop,
                                  std::function<void(int64_t, int64_t)> iop,
                                  std::function<void(std::string, std::string)> sop)
        {
            auto b = interpreter->pop();
            auto a = interpreter->pop();

            if (either_is<std::string>(a, b))
            {
                auto str_a = as_string(interpreter, a);
                auto str_b = as_string(interpreter, b);

                sop(str_a, str_b);
            }
            else if (either_is<double>(a, b))
            {
                auto a_num = as_numeric<double>(interpreter, a);
                auto b_num = as_numeric<double>(interpreter, b);

                dop(a_num, b_num);
            }
            else
            {
                auto a_num = as_numeric<int64_t>(interpreter, a);
                auto b_num = as_numeric<int64_t>(interpreter, b);

                iop(a_num, b_num);
            }
        }



        DataObjectDefinitionPtr make_location_info_definition()
        {
            auto location_definition = std::make_shared<DataObjectDefinition>();

            location_definition->name = "sorth.location";
            location_definition->fieldNames.push_back("path");
            location_definition->fieldNames.push_back("line");
            location_definition->fieldNames.push_back("column");

            return location_definition;
        }


        DataObjectDefinitionPtr make_word_info_definition()
        {
            auto word_definition = std::make_shared<DataObjectDefinition>();

            word_definition->name = "sorth.word";
            word_definition->fieldNames.push_back("name");
            word_definition->fieldNames.push_back("is_immediate");
            word_definition->fieldNames.push_back("is_scripted");
            word_definition->fieldNames.push_back("description");
            word_definition->fieldNames.push_back("signature");
            word_definition->fieldNames.push_back("handler_index");
            word_definition->fieldNames.push_back("location");

            return word_definition;
        }


        DataObjectDefinitionPtr location_definition = make_location_info_definition();
        DataObjectDefinitionPtr word_info_definition = make_word_info_definition();


        void register_word_info_struct(const Location& location, InterpreterPtr& interpreter)
        {
            create_data_definition_words(location, interpreter, location_definition, true);
            create_data_definition_words(location, interpreter, word_info_definition, true);
        }


        DataObjectPtr make_word_info_instance(InterpreterPtr& interpreter,
                                              std::string& name,
                                              Word& word)
        {
            auto new_location = make_data_object(interpreter, location_definition);
            new_location->fields[0] = word.location.get_path()->string();
            new_location->fields[1] = (int64_t)word.location.get_line();
            new_location->fields[2] = (int64_t)word.location.get_column();

            auto new_word = make_data_object(interpreter, word_info_definition);
            new_word->fields[0] = name;
            new_word->fields[1] = word.is_immediate;
            new_word->fields[2] = word.is_scripted;
            new_word->fields[3] = word.description ? *word.description : "";
            new_word->fields[4] = word.signature ? *word.signature : "";
            new_word->fields[5] = (int64_t)word.handler_index;
            new_word->fields[6] = new_location;

            return new_word;
        }



        Token& get_next_token(InterpreterPtr& interpreter)
        {
            auto& current_token = interpreter->constructor().current_token;
            auto& input_tokens = interpreter->constructor().input_tokens;

            if ((current_token + 1) >= input_tokens.size())
            {
                throw_error(interpreter, "Trying to read past end of token stream.");
            }

            ++current_token;
            auto& token = input_tokens[current_token];

            return token;
        }

    }


    void word_reset(InterpreterPtr& interpreter)
    {
        // Once the reset occurs any handler registered has probably ceased to exist.  So,
        // unregister it now.
        at_exit_interpreter = nullptr;
        at_exit_value = (int64_t)0;

        interpreter->release_context();
        interpreter->clear_stack();

        reset_ffi();

        interpreter->mark_context();
    }


    void word_include(InterpreterPtr& interpreter)
    {
        auto path = as_string(interpreter, interpreter->pop());
        interpreter->process_source(interpreter->find_file(path));
    }


    void word_include_im(InterpreterPtr& interpreter)
    {
        auto& token = get_next_token(interpreter);
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
                auto token = get_next_token(interpreter);

                while (!is_one_of(token.text, words))
                {
                    token = get_next_token(interpreter);
                }

                return token.text;
            };

        auto compile_until = [&](std::vector<std::string> words) -> std::string
            {
                auto token = get_next_token(interpreter);

                while (!is_one_of(token.text, words))
                {
                    interpreter->constructor().compile_token(token);
                    token = get_next_token(interpreter);
                }

                return token.text;
            };

        auto result = as_numeric<bool>(interpreter, interpreter->pop());

        if (result)
        {
            auto found = compile_until({ "[else]", "[then]" });

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
                compile_until({ "[then]" });
            }
        }
    }


    std::thread::id pop_as_thread_id(InterpreterPtr interpreter)
    {
        auto value = interpreter->pop();

        if (!std::holds_alternative<std::thread::id>(value))
        {
            throw_error(interpreter, "Value not a thread id.");
        }

        return std::get<std::thread::id>(value);
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
        auto name = as_string(interpreter, interpreter->pop());
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
        auto id = pop_as_thread_id(interpreter);
        auto value = interpreter->pop();

        interpreter->thread_push_input(id, value);
    }


    void word_thread_pop_from(InterpreterPtr& interpreter)
    {
        auto id = pop_as_thread_id(interpreter);
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


    void insert_user_instruction(InterpreterPtr& interpreter, const OperationCode& op)
    {
        auto& constructor = interpreter->constructor();
        auto& code = constructor.stack.top().code;

        if (!constructor.user_is_inserting_at_beginning)
        {
            code.push_back(op);
        }
        else
        {
            code.insert(code.begin(), op);
        }
    }


    void word_op_def_variable(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        if (std::holds_alternative<Token>(value))
        {
            value = std::get<Token>(value).text;
        }

        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::def_variable,
                .value = value
            });
    }


    void word_op_def_constant(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        if (std::holds_alternative<Token>(value))
        {
            value = std::get<Token>(value).text;
        }

        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::def_constant,
                .value = value
            });
    }


    void word_op_read_variable(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::read_variable,
                .value = (int64_t)0
            });
    }


    void word_op_write_variable(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::write_variable,
                .value = (int64_t)0
            });
    }


    void word_op_execute(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        if (std::holds_alternative<Token>(value))
        {
            value = std::get<Token>(value).text;
        }

        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::execute,
                .value = value
            });
    }


    void word_op_push_constant_value(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::push_constant_value,
                .value = interpreter->pop()
            });
    }


    void word_mark_loop_exit(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::mark_loop_exit,
                .value = interpreter->pop()
            });
    }


    void word_unmark_loop_exit(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::unmark_loop_exit,
                .value = (int64_t)0
            });
    }


    void word_op_mark_catch(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::mark_catch,
                .value = interpreter->pop()
            });
    }


    void word_op_unmark_catch(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::unmark_catch,
                .value = (int64_t)0
            });
    }


    void word_op_mark_context(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::mark_context,
                .value = (int64_t)0
            });
    }


    void word_op_release_context(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::release_context,
                .value = (int64_t)0
            });
    }


    void word_op_jump(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::jump,
                .value = interpreter->pop()
            });
    }


    void word_op_jump_if_zero(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::jump_if_zero,
                .value = interpreter->pop()
            });
    }


    void word_op_jump_if_not_zero(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::jump_if_not_zero,
                .value = interpreter->pop()
            });
    }


    void word_jump_loop_start(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::jump_loop_start,
                .value = (int64_t)0
            });
    }


    void word_jump_loop_exit(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::jump_loop_exit,
                .value = (int64_t)0
            });
    }


    void word_op_jump_target(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::jump_target,
                .value = interpreter->pop()
            });
    }


    void word_code_new_block(InterpreterPtr& interpreter)
    {
        interpreter->constructor().stack.push({});
    }


    void word_code_drop_stack_block(InterpreterPtr& interpreter)
    {
        interpreter->constructor().stack.pop();
    }


    void word_code_execute_stack_block(InterpreterPtr& interpreter)
    {
        auto name = as_string(interpreter, interpreter->pop());
        interpreter->execute_code(name, interpreter->constructor().stack.top().code);
    }


    void word_code_merge_stack_block(InterpreterPtr& interpreter)
    {
        auto& stack = interpreter->constructor().stack;
        auto top_code = stack.top().code;

        stack.pop();

        stack.top().code.insert(stack.top().code.end(), top_code.begin(), top_code.end());
    }


    void word_code_pop_stack_block(InterpreterPtr& interpreter)
    {
        interpreter->push(interpreter->constructor().stack.top().code);
        interpreter->constructor().stack.pop();
    }


    void word_code_push_stack_block(InterpreterPtr& interpreter)
    {
        auto top = interpreter->pop();

        throw_error_if(!std::holds_alternative<ByteCode>(top),
                       interpreter,
                       "Expected a byte code block.");

        interpreter->constructor().stack.push({ .code = std::get<ByteCode>(top) });
    }


    void word_code_stack_block_size(InterpreterPtr& interpreter)
    {
        interpreter->push((int64_t)interpreter->constructor().stack.top().code.size());
    }


    void word_code_resolve_jumps(InterpreterPtr& interpreter)
    {
        auto is_jump = [](const OperationCode& code) -> bool
            {
                return    (code.id == OperationCode::Id::jump)
                       || (code.id == OperationCode::Id::jump_if_not_zero)
                       || (code.id == OperationCode::Id::jump_if_zero)
                       || (code.id == OperationCode::Id::mark_loop_exit)
                       || (code.id == OperationCode::Id::mark_catch);
            };
        auto& top_code = interpreter->constructor().stack.top().code;

        std::list<size_t> jump_indicies;
        std::unordered_map<std::string, size_t> jump_targets;

        for (size_t i = 0; i < top_code.size(); ++i)
        {
            if (is_jump(top_code[i]))
            {
                jump_indicies.push_back(i);
            }
            else if (top_code[i].id == OperationCode::Id::jump_target)
            {
                jump_targets.insert({ as_string(interpreter, top_code[i].value), i });
                top_code[i].value = (int64_t)0;
            }
        }

        for (auto jump_index : jump_indicies)
        {
            auto& jump_op = top_code[jump_index];

            auto jump_name = as_string(interpreter, jump_op.value);
            auto iter = jump_targets.find(jump_name);

            if (iter != jump_targets.end())
            {
                auto target_index = iter->second;
                jump_op.value = (int64_t)target_index - (int64_t)jump_index;
            }
        }
    }


    void word_code_compile_until_words(InterpreterPtr& interpreter)
    {
        auto count = as_numeric<int64_t>(interpreter, interpreter->pop());
        std::vector<std::string> word_list;

        for (int64_t i = 0; i < count; ++i)
        {
            word_list.push_back(as_string(interpreter, interpreter->pop()));
        }

        auto is_one_of_words = [&](const std::string& match) -> std::tuple<bool, std::string>
            {
                for (const auto& word : word_list)
                {
                    if (word == match)
                    {
                        return { true, word };
                    }
                }

                return { false, "" };
            };

        auto& current_token = interpreter->constructor().current_token;
        auto& input_tokens = interpreter->constructor().input_tokens;

        auto start_location = input_tokens[current_token].location;

        for (++current_token; current_token < input_tokens.size(); ++current_token)
        {
            auto& token = input_tokens[current_token];
            auto [found, word] = is_one_of_words(token.text);

            if ((found) && (token.type == Token::Type::word))
            {
                interpreter->push(word);
                return;
            }

            interpreter->constructor().compile_token(token);
        }

        std::string message;

        if (count == 1)
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

        throw_error(start_location, message);
    }


    void word_code_insert_at_front(InterpreterPtr& interpreter)
    {
        interpreter->constructor().user_is_inserting_at_beginning =
                                                  as_numeric<bool>(interpreter, interpreter->pop());
    }


    void word_code_execute_source(InterpreterPtr& interpreter)
    {
        auto code = as_string(interpreter, interpreter->pop());
        interpreter->process_source(code);
    }


    void word_token_is_string(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        if (std::holds_alternative<Token>(value))
        {
            auto token = std::get<Token>(value);
            interpreter->push(token.type == Token::Type::string);
        }
        else
        {
            throw_error(interpreter, "Value not a token.");
        }
    }


    void word_token_text(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        if (std::holds_alternative<Token>(value))
        {
            interpreter->push(std::get<Token>(value).text);
        }
        else
        {
            throw_error(interpreter, "Value not a token.");
        }
    }


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
                    .id = OperationCode::Id::push_constant_value,
                    .value = (int64_t)word.handler_index
                });
        }
        else
        {
            interpreter->constructor().stack.top().code.push_back({
                    .id = OperationCode::Id::word_index,
                    .value = name
                });
        }
    }


    void word_execute(InterpreterPtr& interpreter)
    {
        auto word_value = interpreter->pop();

        if (is_numeric(word_value))
        {
            auto index = as_numeric<int64_t>(interpreter, word_value);
            auto& handler = interpreter->get_handler_info(index);

            handler.function(interpreter);
        }
        else if (is_string(word_value))
        {
            auto name = as_string(interpreter, word_value);

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
                .id = OperationCode::Id::word_exists,
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
        auto found = as_numeric<bool>(interpreter, interpreter->pop());

        interpreter->push(!found);
    }


    void word_throw(InterpreterPtr& interpreter)
    {
        throw_error(interpreter,
            as_string(interpreter, interpreter->pop()));
    }


    void word_start_word(InterpreterPtr& interpreter)
    {
        auto& current_token = interpreter->constructor().current_token;
        auto& input_tokens = interpreter->constructor().input_tokens;

        ++current_token;
        auto& name = input_tokens[current_token].text;
        auto& location = input_tokens[current_token].location;

        interpreter->constructor().stack.push({
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
        auto construction = interpreter->constructor().stack.top();
        interpreter->constructor().stack.pop();

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
                interpreter->constructor().word_jit_cache[construction.name] = construction;

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
        interpreter->constructor().stack.top().is_immediate = true;
    }


    void word_hidden(InterpreterPtr& interpreter)
    {
        interpreter->constructor().stack.top().is_hidden = true;
    }


    void word_contextless(InterpreterPtr& interpreter)
    {
        interpreter->constructor().stack.top().is_context_managed = false;
    }


    void word_description(InterpreterPtr& interpreter)
    {
        auto& current_token = interpreter->constructor().current_token;
        auto& input_tokens = interpreter->constructor().input_tokens;

        ++current_token;

        throw_error_if(current_token >= input_tokens.size(), interpreter,
            "Unexpected end to token stream.");

        auto& token = input_tokens[current_token];

        throw_error_if(token.type != Token::Type::string,
                       interpreter,
                       "Expected the description to be a string.");

        interpreter->constructor().stack.top().description = token.text;
    }


    void word_signature(InterpreterPtr& interpreter)
    {
        auto& current_token = interpreter->constructor().current_token;
        auto& input_tokens = interpreter->constructor().input_tokens;

        ++current_token;

        throw_error_if(current_token >= input_tokens.size(), interpreter,
                       "Unexpected end to token stream.");

        auto& token = input_tokens[current_token];

        throw_error_if(token.type != Token::Type::string,
                       interpreter,
                       "Expected the signature to be a string.");

        interpreter->constructor().stack.top().signature = token.text;
    }


    void word_value_is_number(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(   std::holds_alternative<int64_t>(value)
                          || std::holds_alternative<double>(value));
    }


    void word_value_is_boolean(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<bool>(value));
    }


    void word_value_is_string(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<std::string>(value));
    }


    void word_value_is_structure(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<DataObjectPtr>(value));
    }


    void word_value_is_array(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<ArrayPtr>(value));
    }


    void word_value_is_buffer(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<ByteBufferPtr>(value));
    }


    void word_value_is_hash_table(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<HashTablePtr>(value));
    }


    void word_value_copy(InterpreterPtr& interpreter)
    {
        auto original = interpreter->pop();
        interpreter->push(deep_copy_value(interpreter, original));
    }


    void word_string_length(InterpreterPtr& interpreter)
    {
        auto string = as_string(interpreter, interpreter->pop());

        interpreter->push((int64_t)string.size());
    }


    void word_string_insert(InterpreterPtr& interpreter)
    {
        auto string = as_string(interpreter, interpreter->pop());
        auto position = (size_t)as_numeric<int64_t>(interpreter, interpreter->pop());
        auto sub_string = as_string(interpreter, interpreter->pop());

        string.insert(position, sub_string);

        interpreter->push(string);
    }


    void word_string_remove(InterpreterPtr& interpreter)
    {
        auto string = as_string(interpreter, interpreter->pop());
        auto start = (size_t)as_numeric<int64_t>(interpreter, interpreter->pop());
        auto count = (size_t)as_numeric<int64_t>(interpreter, interpreter->pop());

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
        auto string = as_string(interpreter, interpreter->pop());
        auto search_str = as_string(interpreter, interpreter->pop());

        interpreter->push((int64_t)string.find(search_str, 0));
    }


    void word_string_index_read(InterpreterPtr& interpreter)
    {
        auto string = as_string(interpreter, interpreter->pop());
        auto position = (size_t)as_numeric<int64_t>(interpreter, interpreter->pop());

        if ((position < 0) || (position >= string.size()))
        {
            throw_error(interpreter, "String index out of range.");
        }

        std::string new_str(1, string[position]);
        interpreter->push(new_str);
    }


    void word_string_add(InterpreterPtr& interpreter)
    {
        auto str_b = as_string(interpreter, interpreter->pop());
        auto str_a = as_string(interpreter, interpreter->pop());

        interpreter->push(str_a + str_b);
    }


    void word_string_to_number(InterpreterPtr& interpreter)
    {
        auto string = as_string(interpreter, interpreter->pop());

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


    void word_data_definition(InterpreterPtr& interpreter)
    {
        Location location = interpreter->get_current_location();

        bool found_initializers = as_numeric<bool>(interpreter, interpreter->pop());
        bool is_hidden = as_numeric<bool>(interpreter, interpreter->pop());
        ArrayPtr fields = as_array(interpreter, interpreter->pop());
        std::string name = as_string(interpreter, interpreter->pop());
        ArrayPtr defaults;

        if (found_initializers)
        {
            defaults = as_array(interpreter, interpreter->pop());
        }

        // Create the definition object.
        auto definition_ptr = create_data_definition(interpreter,
                                                     name,
                                                     fields,
                                                     defaults,
                                                     is_hidden);

        // Create the words to allow the script to access this definition.  The word
        // <definition_name>.new will always hold a base reference to our definition object.
        create_data_definition_words(location, interpreter, definition_ptr, is_hidden);
    }


    void word_read_field(InterpreterPtr& interpreter)
    {
        auto var = interpreter->pop();

        throw_error_if(!std::holds_alternative<DataObjectPtr>(var),
                       interpreter, "Expected data object.");

        auto object = std::get<DataObjectPtr>(var);

        var = interpreter->pop();
        auto field_index = as_numeric<int64_t>(interpreter, var);

        interpreter->push(object->fields[field_index]);
    }


    void word_write_field(InterpreterPtr& interpreter)
    {
        auto var = interpreter->pop();

        throw_error_if(!std::holds_alternative<DataObjectPtr>(var),
                       interpreter, "Expected data object.");

        auto object = std::get<DataObjectPtr>(var);

        var = interpreter->pop();
        auto field_index = as_numeric<int64_t>(interpreter, var);

        var = interpreter->pop();
        object->fields[field_index] = var;
    }


    void word_structure_iterate(InterpreterPtr& interpreter)
    {
        auto var = interpreter->pop();

        throw_error_if(!std::holds_alternative<DataObjectPtr>(var),
                       interpreter, "Expected data object.");

        auto object = std::get<DataObjectPtr>(var);
        auto word_index = as_numeric<int64_t>(interpreter, interpreter->pop());

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
        auto var = interpreter->pop();

        throw_error_if(!std::holds_alternative<DataObjectPtr>(var),
                       interpreter, "Expected data object.");

        auto object = std::get<DataObjectPtr>(var);
        auto field_name = as_string(interpreter, interpreter->pop());

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
        auto var = interpreter->pop();

        throw_error_if(!std::holds_alternative<DataObjectPtr>(var),
                       interpreter, "Expected data object.");

        auto a = std::get<DataObjectPtr>(var);

        var = interpreter->pop();

        throw_error_if(!std::holds_alternative<DataObjectPtr>(var),
                       interpreter, "Expected data object.");

        auto b = std::get<DataObjectPtr>(var);

        interpreter->push(a == b);
    }


    void word_array_new(InterpreterPtr& interpreter)
    {
        auto count = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto array_ptr = std::make_shared<Array>(count);

        interpreter->push(array_ptr);
    }


    void word_array_size(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());

        interpreter->push(array->size());
    }


    void word_array_write_index(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());
        auto index = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto new_value = interpreter->pop();

        throw_if_out_of_bounds(interpreter, index, array->size(), "Array");

        (*array)[index] = new_value;
    }


    void word_array_read_index(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());
        auto index = as_numeric<int64_t>(interpreter, interpreter->pop());

        throw_if_out_of_bounds(interpreter, index, array->size(), "Array");

        interpreter->push((*array)[index]);
    }


    void word_array_insert(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());
        auto index = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto value = interpreter->pop();

        array->insert(index, value);
    }


    void word_array_delete(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());
        auto index = as_numeric<int64_t>(interpreter, interpreter->pop());

        throw_if_out_of_bounds(interpreter, index, array->size(), "Array");

        array->remove(index);
    }


    void word_array_resize(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());
        auto new_size = as_numeric<int64_t>(interpreter, interpreter->pop());

        array->resize(new_size);
    }


    void word_array_plus(InterpreterPtr& interpreter)
    {
        auto array_src = as_array(interpreter, interpreter->pop());
        auto array_dest = as_array(interpreter, interpreter->pop());

        auto orig_size = array_dest->size();
        auto new_size = orig_size + array_src->size();

        array_dest->resize(new_size);

        for (auto i = orig_size; i < new_size; ++i)
        {
            (*array_dest)[i] = deep_copy_value(interpreter, (*array_src)[i - orig_size]);
        }

        interpreter->push(array_dest);
    }



    void word_array_compare(InterpreterPtr& interpreter)
    {
        auto array_a = as_array(interpreter, interpreter->pop());
        auto array_b = as_array(interpreter, interpreter->pop());

        interpreter->push(array_a == array_b);
    }


    void word_push_front(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());
        auto value = interpreter->pop();

        array->push_front(value);
    }

    void word_push_back(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());
        auto value = interpreter->pop();

        array->push_back(value);
    }

    void word_pop_front(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());

        interpreter->push(array->pop_front(interpreter));
    }

    void word_pop_back(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());

        interpreter->push(array->pop_back(interpreter));
    }



    void word_buffer_new(InterpreterPtr& interpreter)
    {
        auto size = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto buffer = std::make_shared<ByteBuffer>(size);

        interpreter->push(buffer);
    }


    void word_buffer_write_int(InterpreterPtr& interpreter)
    {
        auto byte_size = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto buffer = as_byte_buffer(interpreter, interpreter->pop());
        auto value = as_numeric<int64_t>(interpreter, interpreter->pop());

        check_buffer_index(interpreter, buffer, byte_size);
        buffer->write_int(byte_size, value);
    }


    void word_buffer_read_int(InterpreterPtr& interpreter)
    {
        auto is_signed = as_numeric<bool>(interpreter, interpreter->pop());
        auto byte_size = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto buffer = as_byte_buffer(interpreter, interpreter->pop());

        check_buffer_index(interpreter, buffer, byte_size);
        interpreter->push(buffer->read_int(byte_size, is_signed));
    }


    void word_buffer_write_float(InterpreterPtr& interpreter)
    {
        auto byte_size = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto buffer = as_byte_buffer(interpreter, interpreter->pop());
        auto value = as_numeric<double>(interpreter, interpreter->pop());

        check_buffer_index(interpreter, buffer, byte_size);
        buffer->write_float(byte_size, value);
    }


    void word_buffer_read_float(InterpreterPtr& interpreter)
    {
        auto byte_size = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto buffer = as_byte_buffer(interpreter, interpreter->pop());

        check_buffer_index(interpreter, buffer, byte_size);
        interpreter->push(buffer->read_float(byte_size));
    }


    void word_buffer_write_string(InterpreterPtr& interpreter)
    {
        auto max_size = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto buffer = as_byte_buffer(interpreter, interpreter->pop());
        auto value = as_string(interpreter, interpreter->pop());

        check_buffer_index(interpreter, buffer, max_size);
        buffer->write_string(value, max_size);
    }


    void word_buffer_read_string(InterpreterPtr& interpreter)
    {
        auto max_size = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto buffer = as_byte_buffer(interpreter, interpreter->pop());

        check_buffer_index(interpreter, buffer, max_size);
        interpreter->push(buffer->read_string(max_size));
    }


    void word_buffer_set_position(InterpreterPtr& interpreter)
    {
        auto buffer = as_byte_buffer(interpreter, interpreter->pop());
        auto new_position = as_numeric<int64_t>(interpreter, interpreter->pop());

        buffer->set_position(new_position);
    }


    void word_buffer_get_position(InterpreterPtr& interpreter)
    {
        auto buffer = as_byte_buffer(interpreter, interpreter->pop());

        interpreter->push(buffer->position());
    }


    void word_hash_table_new(InterpreterPtr& interpreter)
    {
        auto table = std::make_shared<HashTable>();
        interpreter->push(table);
    }


    void word_hash_table_insert(InterpreterPtr& interpreter)
    {
        auto table = as_hash_table(interpreter, interpreter->pop());
        auto key = interpreter->pop();
        auto value = interpreter->pop();

        table->insert(key, value);
    }


    void word_hash_table_find(InterpreterPtr& interpreter)
    {
        auto table = as_hash_table(interpreter, interpreter->pop());
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
        auto table = as_hash_table(interpreter, interpreter->pop());
        auto key = interpreter->pop();

        auto [ found, value ] = table->get(key);

        interpreter->push(found);
    }


    void word_hash_plus(InterpreterPtr& interpreter)
    {
        auto hash_src = as_hash_table(interpreter, interpreter->pop());
        auto hash_dest = as_hash_table(interpreter, interpreter->pop());

        for (auto entry : hash_src->get_items())
        {
            auto key = entry.first;
            auto value = entry.second;

            hash_dest->insert(deep_copy_value(interpreter, key),
                              deep_copy_value(interpreter, value));
        }

        interpreter->push(hash_dest);
    }


    void word_hash_compare(InterpreterPtr& interpreter)
    {
        auto hash_a = as_hash_table(interpreter, interpreter->pop());
        auto hash_b = as_hash_table(interpreter, interpreter->pop());

        interpreter->push(hash_a == hash_b);
    }


    void word_hash_table_size(InterpreterPtr& interpreter)
    {
        auto hash = as_hash_table(interpreter, interpreter->pop());

        interpreter->push(hash->size());
    }


    void word_hash_table_iterate(InterpreterPtr& interpreter)
    {
        auto table = as_hash_table(interpreter, interpreter->pop());
        auto word_index = as_numeric<int64_t>(interpreter, interpreter->pop());

        auto& handler = interpreter->get_handler_info(word_index);

        for (const auto& item : table->get_items())
        {
            interpreter->push(item.first);
            interpreter->push(item.second);

            handler.function(interpreter);
        }
    }


    void word_add(InterpreterPtr& interpreter)
    {
        string_or_numeric_op(interpreter,
                             [&](auto a, auto b) { interpreter->push(a + b); },
                             [&](auto a, auto b) { interpreter->push(a + b); },
                             [&](auto a, auto b) { interpreter->push(a + b); });
    }


    void word_subtract(InterpreterPtr& interpreter)
    {
        math_op(interpreter,
                [](auto a, auto b) -> auto { return a - b; },
                [](auto a, auto b) -> auto { return a - b; });
    }


    void word_multiply(InterpreterPtr& interpreter)
    {
        math_op(interpreter,
                [](auto a, auto b) -> auto { return a * b; },
                [](auto a, auto b) -> auto { return a * b; });
    }


    void word_divide(InterpreterPtr& interpreter)
    {
        math_op(interpreter,
                [](auto a, auto b) -> auto { return a / b; },
                [](auto a, auto b) -> auto { return a / b; });
    }


    void word_mod(InterpreterPtr& interpreter)
    {
        auto b = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto a = as_numeric<int64_t>(interpreter, interpreter->pop());

        interpreter->push(a % b);
    }


    void word_logic_and(InterpreterPtr& interpreter)
    {
        logic_op(interpreter, [](auto a, auto b) { return a && b; });
    }


    void word_logic_or(InterpreterPtr& interpreter)
    {
        logic_op(interpreter, [](auto a, auto b) { return a || b; });
    }


    void word_logic_not(InterpreterPtr& interpreter)
    {
        auto value = as_numeric<bool>(interpreter, interpreter->pop());

        interpreter->push(!value);
    }


    void word_bit_and(InterpreterPtr& interpreter)
    {
        logic_bit_op(interpreter, [](auto a, auto b) { return a & b; });
    }


    void word_bit_or(InterpreterPtr& interpreter)
    {
        logic_bit_op(interpreter, [](auto a, auto b) { return a | b; });
    }


    void word_bit_xor(InterpreterPtr& interpreter)
    {
        logic_bit_op(interpreter, [](auto a, auto b) { return a ^ b; });
    }


    void word_bit_not(InterpreterPtr& interpreter)
    {
        auto top = interpreter->pop();
        auto value = as_numeric<int64_t>(interpreter, top);

        value = ~value;

        interpreter->push(value);
    }


    void word_bit_left_shift(InterpreterPtr& interpreter)
    {
        logic_bit_op(interpreter, [](auto value, auto amount) { return value << amount; });
    }


    void word_bit_right_shift(InterpreterPtr& interpreter)
    {
        logic_bit_op(interpreter, [](auto value, auto amount) { return value >> amount; });
    }


    void word_equal(InterpreterPtr& interpreter)
    {
        string_or_numeric_op(interpreter,
                             [&](auto a, auto b) { interpreter->push((bool)(a == b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a == b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a == b)); });
    }


    void word_greater_equal(InterpreterPtr& interpreter)
    {
        string_or_numeric_op(interpreter,
                             [&](auto a, auto b) { interpreter->push((bool)(a >= b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a >= b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a >= b)); });
    }


    void word_less_equal(InterpreterPtr& interpreter)
    {
        string_or_numeric_op(interpreter,
                             [&](auto a, auto b) { interpreter->push((bool)(a <= b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a <= b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a <= b)); });
    }


    void word_greater(InterpreterPtr& interpreter)
    {
        string_or_numeric_op(interpreter,
                             [&](auto a, auto b) { interpreter->push((bool)(a > b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a > b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a > b)); });
    }


    void word_less(InterpreterPtr& interpreter)
    {
        string_or_numeric_op(interpreter,
                             [&](auto a, auto b) { interpreter->push((bool)(a < b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a < b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a < b)); });
    }


    void word_dup(InterpreterPtr& interpreter)
    {
        auto next = interpreter->pop();

        interpreter->push(next);
        interpreter->push(next);
    }


    void word_drop(InterpreterPtr& interpreter)
    {
        interpreter->pop();
    }


    void word_swap(InterpreterPtr& interpreter)
    {
        auto a = interpreter->pop();
        auto b = interpreter->pop();

        interpreter->push(a);
        interpreter->push(b);
    }


    void word_over(InterpreterPtr& interpreter)
    {
        auto a = interpreter->pop();
        auto b = interpreter->pop();

        interpreter->push(a);
        interpreter->push(b);
        interpreter->push(a);
    }


    void word_rot(InterpreterPtr& interpreter)
    {
        auto c = interpreter->pop();
        auto b = interpreter->pop();
        auto a = interpreter->pop();

        interpreter->push(c);
        interpreter->push(a);
        interpreter->push(b);
    }


    void word_stack_depth(InterpreterPtr& interpreter)
    {
        interpreter->push(interpreter->depth());
    }


    void word_pick(InterpreterPtr& interpreter)
    {
        auto index = as_numeric<int64_t>(interpreter, interpreter->pop());

        interpreter->push(interpreter->pick(index));
    }


    void word_push_to(InterpreterPtr& interpreter)
    {
        auto index = as_numeric<int64_t>(interpreter, interpreter->pop());
        interpreter->push_to(index);
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





    void word_hex(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        std::stringstream stream;

        if (is_string(value))
        {
            auto string_value = as_string(interpreter, value);

            for (auto next : string_value)
            {
                stream << std::hex << (int)next << std::dec;
            }
        }
        else
        {
            auto int_value = as_numeric<int64_t>(interpreter, value);
            stream << std::hex << int_value << std::dec << " ";
        }

        interpreter->push(stream.str());
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
        interpreter->push(std::string(VERSION));
    }


    void word_get_compiler_version(InterpreterPtr& interpreter)
    {
        interpreter->push(std::string(MESSAGE));
    }


    void word_show_bytecode(InterpreterPtr& interpreter)
    {
        interpreter->showing_bytecode() = as_numeric<bool>(interpreter, interpreter->pop());
    }


    void word_show_run_code(InterpreterPtr& interpreter)
    {
        interpreter->showing_run_code() = as_numeric<bool>(interpreter, interpreter->pop());
    }


    void word_show_word_bytecode(InterpreterPtr& interpreter)
    {
        std::string name;
        auto value = interpreter->pop();

        if (is_string(value))
        {
            name = as_string(interpreter, value);
        }
        else if (is_numeric(value))
        {
            auto index = as_numeric<int64_t>(interpreter, value);
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

        if (is_string(value))
        {
            name = as_string(interpreter, value);
        }
        else if (is_numeric(value))
        {
            auto index = as_numeric<int64_t>(interpreter, value);
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

        if (is_string(value))
        {
            name = as_string(interpreter, value);
        }
        else if (is_numeric(value))
        {
            auto index = as_numeric<int64_t>(interpreter, value);
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


    void register_builtin_words(InterpreterPtr& interpreter)
    {
        // Manage the interpreter state.
        ADD_NATIVE_WORD(interpreter, "reset", word_reset,
            "Reset the interpreter to it's default state.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "include", word_include,
            "Include and execute another source file.",
            "source_path -- ");

        ADD_IMMEDIATE_WORD(interpreter, "[include]", word_include_im,
            "Include and execute another source file.",
            "[include] path/to/file.f");

        ADD_IMMEDIATE_WORD(interpreter, "[if]", word_if_im,
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


        // Words for creating new bytecode.
        ADD_NATIVE_WORD(interpreter, "op.def_variable", word_op_def_variable,
            "Insert this instruction into the byte stream.",
            "new-name -- ");

        ADD_NATIVE_WORD(interpreter, "op.def_constant", word_op_def_constant,
            "Insert this instruction into the byte stream.",
            "new-name -- ");

        ADD_NATIVE_WORD(interpreter, "op.read_variable", word_op_read_variable,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.write_variable", word_op_write_variable,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.execute", word_op_execute,
            "Insert this instruction into the byte stream.",
            "index -- ");

        ADD_NATIVE_WORD(interpreter, "op.push_constant_value", word_op_push_constant_value,
            "Insert this instruction into the byte stream.",
            "value -- ");

        ADD_NATIVE_WORD(interpreter, "op.mark_loop_exit", word_mark_loop_exit,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "op.unmark_loop_exit", word_unmark_loop_exit,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.mark_catch", word_op_mark_catch,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "op.unmark_catch", word_op_unmark_catch,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.mark_context", word_op_mark_context,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.release_context", word_op_release_context,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump", word_op_jump,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump_if_zero", word_op_jump_if_zero,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump_if_not_zero", word_op_jump_if_not_zero,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump_loop_start", word_jump_loop_start,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump_loop_exit", word_jump_loop_exit,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump_target", word_op_jump_target,
            "Insert this instruction into the byte stream.",
            "identifier -- ");


        ADD_NATIVE_WORD(interpreter, "code.new_block", word_code_new_block,
            "Create a new sub-block on the code generation stack.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "code.drop_stack_block", word_code_drop_stack_block,
            "Drop the code object that's at the top of the construction stack.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "code.execute_stack_block", word_code_execute_stack_block,
            "Take the top code block of the current construction and execute it now.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "code.merge_stack_block", word_code_merge_stack_block,
            "Merge the top code block into the one below.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "code.pop_stack_block", word_code_pop_stack_block,
            "Pop a code block off of the code stack and onto the data stack.",
            " -- code_block");

        ADD_NATIVE_WORD(interpreter, "code.push_stack_block", word_code_push_stack_block,
            "Pop a block from the data stack and back onto the code stack.",
            "code_block -- ");

        ADD_NATIVE_WORD(interpreter, "code.stack_block_size@", word_code_stack_block_size,
            "Read the size of the code block at the top of the stack.",
            " -- code_size");

        ADD_NATIVE_WORD(interpreter, "code.resolve_jumps", word_code_resolve_jumps,
            "Resolve all of the jumps in the top code block.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "code.compile_until_words", word_code_compile_until_words,
            "Compile words until one of the given words is found.",
            "words... word_count -- found_word");

        ADD_NATIVE_WORD(interpreter, "code.insert_at_front", word_code_insert_at_front,
            "When true new instructions are added beginning of the block.",
            "bool -- ");


        ADD_NATIVE_WORD(interpreter, "code.execute_source", word_code_execute_source,
            "Interpret and execute a string like it is source code.",
            "string_to_execute -- ???");


        // Token words.
        ADD_NATIVE_WORD(interpreter, "token.is_string?", word_token_is_string,
            "Does the token represent a string value?",
            "token -- bool");

        ADD_NATIVE_WORD(interpreter, "token.text@", word_token_text,
            "Get the text value from a token.",
            "token -- string");


        // Word words.
        ADD_NATIVE_WORD(interpreter, "word", word_word,
            "Get the next word in the token stream.",
            " -- next_word");

        ADD_NATIVE_WORD(interpreter, "words.get{}", word_get_word_table,
            "Get a copy of the word table as it exists at time of calling.",
            " -- all_defined_words");

        ADD_IMMEDIATE_WORD(interpreter, "`", word_word_index,
            "Get the index of the next word.",
            " -- index");

        ADD_NATIVE_WORD(interpreter, "execute", word_execute,
            "Execute a word name or index.",
            "word_name_or_index -- ???");

        ADD_IMMEDIATE_WORD(interpreter, "defined?", word_is_defined,
            "Is the given word defined?",
            " -- bool");

        ADD_IMMEDIATE_WORD(interpreter, "[defined?]", word_is_defined_im,
            "Evaluate at compile time, is the given word defined?",
            " -- bool");

        ADD_IMMEDIATE_WORD(interpreter, "[undefined?]", word_is_undefined_im,
            "Evaluate at compile time, is the given word not defined?",
            " -- bool");


        // Exception time
        ADD_NATIVE_WORD(interpreter, "throw", word_throw,
            "Throw an exception with the given message.",
            "message -- ");


        // Creating new words.
        ADD_IMMEDIATE_WORD(interpreter, ":", word_start_word,
            "The start of a new word definition.",
            " -- ");

        ADD_IMMEDIATE_WORD(interpreter, ";", word_end_word,
            "The end of a new word definition.",
            " -- ");

        ADD_IMMEDIATE_WORD(interpreter, "immediate", word_immediate,
            "Mark the current word being built as immediate.",
            " -- ");

        ADD_IMMEDIATE_WORD(interpreter, "hidden", word_hidden,
            "Mark the current word being built as hidden.",
            " -- ");

        ADD_IMMEDIATE_WORD(interpreter, "contextless", word_contextless,
            "Disable automatic context management for the word.",
            " -- ");

        ADD_IMMEDIATE_WORD(interpreter, "description:", word_description,
            "Give a new word it's description.",
            " -- ");

        ADD_IMMEDIATE_WORD(interpreter, "signature:", word_signature,
            "Describe a new word's stack signature.",
            " -- ");


        // Check value types.
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


        // String words.
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

        ADD_NATIVE_WORD(interpreter, "to_string", word_to_string,
            "Convert a value to a string.",
            "value -- string");

        ADD_NATIVE_WORD(interpreter, "string.npos", [](auto& interpreter)
            {
                interpreter->push((int64_t)std::string::npos);
            },
            "Constant value that indicates a search has failed.",
            " -- npos");


        // Data structure support.
        ADD_IMMEDIATE_WORD(interpreter, "#", word_data_definition,
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


        // Array words.
        ADD_NATIVE_WORD(interpreter, "[].new", word_array_new,
            "Create a new array with the given default size.",
            "size -- array");

        ADD_NATIVE_WORD(interpreter, "[].size@", word_array_size,
            "Read the size of the array object.",
            "array -- size");

        ADD_NATIVE_WORD(interpreter, "[]!", word_array_write_index,
            "Write to a value in the array.",
            "value index array -- ");

        ADD_NATIVE_WORD(interpreter, "[]@", word_array_read_index,
            "Read a value from the array.",
            "index array -- value");

        ADD_NATIVE_WORD(interpreter, "[].insert", word_array_insert,
            "Grow an array by inserting a value at the given location.",
            "value index array -- ");

        ADD_NATIVE_WORD(interpreter, "[].delete", word_array_delete,
            "Shrink an array by removing the value at the given location.",
            "index array -- ");

        ADD_NATIVE_WORD(interpreter, "[].size!", word_array_resize,
            "Grow or shrink the array to the new size.",
            "new_size array -- ");

        ADD_NATIVE_WORD(interpreter, "[].+", word_array_plus,
            "Take two arrays and deep copy the contents from the second into the "
            "first.",
            "dest source -- dest");

        ADD_NATIVE_WORD(interpreter, "[].=", word_array_compare,
            "Take two arrays and compare the contents to each other.",
            "dest source -- dest");

        ADD_NATIVE_WORD(interpreter, "[].push_front!", word_push_front,
            "Push a value to the front of an array.",
            "value array -- ");

        ADD_NATIVE_WORD(interpreter, "[].push_back!", word_push_back,
            "Push a value to the end of an array.",
            "value array -- ");

        ADD_NATIVE_WORD(interpreter, "[].pop_front!", word_pop_front,
            "Pop a value from the front of an array.",
            "array -- value");

        ADD_NATIVE_WORD(interpreter, "[].pop_back!", word_pop_back,
            "Pop a value from the back of an array.",
            "array -- value");


        // ByteBuffer operations.
        ADD_NATIVE_WORD(interpreter, "buffer.new", word_buffer_new,
            "Create a new byte buffer.",
            "size -- buffer");


        ADD_NATIVE_WORD(interpreter, "buffer.int!", word_buffer_write_int,
            "Write an integer of a given size to the buffer.",
            "value buffer byte_size -- ");

        ADD_NATIVE_WORD(interpreter, "buffer.int@", word_buffer_read_int,
            "Read an integer of a given size from the buffer.",
            "buffer byte_size is_signed -- value");


        ADD_NATIVE_WORD(interpreter, "buffer.float!", word_buffer_write_float,
            "Write a float of a given size to the buffer.",
            "value buffer byte_size -- ");

        ADD_NATIVE_WORD(interpreter, "buffer.float@", word_buffer_read_float,
            "read a float of a given size from the buffer.",
            "buffer byte_size -- value");


        ADD_NATIVE_WORD(interpreter, "buffer.string!", word_buffer_write_string,
            "Write a string of given size to the buffer.  Padded with 0s if needed.",
            "value buffer size -- ");

        ADD_NATIVE_WORD(interpreter, "buffer.string@", word_buffer_read_string,
            "Read a string of a given max size from the buffer.",
            "buffer size -- value");


        ADD_NATIVE_WORD(interpreter, "buffer.position!", word_buffer_set_position,
            "Set the position of the buffer pointer.",
            "position buffer -- ");

        ADD_NATIVE_WORD(interpreter, "buffer.position@", word_buffer_get_position,
            "Get the position of the buffer pointer.",
            "buffer -- position");


        // HashTable operations.
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


        // Math ops.
        ADD_NATIVE_WORD(interpreter, "+", word_add,
            "Add 2 numbers or strings together.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "-", word_subtract,
            "Subtract 2 numbers.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "*", word_multiply,
            "Multiply 2 numbers.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "/", word_divide,
            "Divide 2 numbers.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "%", word_mod,
            "Mod 2 numbers.",
            "a b -- result");


        // Logical words.
        ADD_NATIVE_WORD(interpreter, "&&", word_logic_and,
            "Logically compare 2 values.",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, "||", word_logic_or,
            "Logically compare 2 values.",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, "'", word_logic_not,
            "Logically invert a boolean value.",
            "bool -- bool");


        // Bitwise operator words.
        ADD_NATIVE_WORD(interpreter, "&", word_bit_and,
            "Bitwise AND two numbers together.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "|", word_bit_or,
            "Bitwise OR two numbers together.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "^", word_bit_xor,
            "Bitwise XOR two numbers together.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "~", word_bit_not,
            "Bitwise NOT a number.",
            "number -- result");

        ADD_NATIVE_WORD(interpreter, "<<", word_bit_left_shift,
            "Shift a numbers bits to the left.",
            "value amount -- result");

        ADD_NATIVE_WORD(interpreter, ">>", word_bit_right_shift,
            "Shift a numbers bits to the right.",
            "value amount -- result");


        // Equality words.
        ADD_NATIVE_WORD(interpreter, "=", word_equal,
            "Are 2 values equal?",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, ">=", word_greater_equal,
            "Is one value greater or equal to another?",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, "<=", word_less_equal,
            "Is one value less than or equal to another?",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, ">", word_greater,
            "Is one value greater than another?",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, "<", word_less,
            "Is one value less than another?",
            "a b -- bool");


        // Stack words.
        ADD_NATIVE_WORD(interpreter, "dup", word_dup,
            "Duplicate the top value on the data stack.",
            "value -- value value");

        ADD_NATIVE_WORD(interpreter, "drop", word_drop,
            "Discard the top value on the data stack.",
            "value -- ");

        ADD_NATIVE_WORD(interpreter, "swap", word_swap,
            "Swap the top 2 values on the data stack.",
            "a b -- b a");

        ADD_NATIVE_WORD(interpreter, "over", word_over,
            "Make a copy of the top value and place the copy under the second.",
            "a b -- b a b");

        ADD_NATIVE_WORD(interpreter, "rot", word_rot,
            "Rotate the top 3 values on the stack.",
            "a b c -- c a b");

        ADD_NATIVE_WORD(interpreter, "stack.depth", word_stack_depth,
            "Get the current depth of the stack.",
            " -- depth");

        ADD_NATIVE_WORD(interpreter, "pick", word_pick,
            "Pick the value n locations down in the stack and push it on the top.",
            "n -- value");

        ADD_NATIVE_WORD(interpreter, "push-to", word_push_to,
            "Pop the top value and push it back into the stack a position from the top.",
            "n -- <updated-stack>>");


        // Some built in constants.
        ADD_NATIVE_WORD(interpreter, "unique_str", word_unique_str,
            "Generate a unique string and push it onto the data stack.",
            " -- string");


        ADD_NATIVE_WORD(interpreter, "exit_success", word_exit_success,
            "Constant value for a process success exit code.",
            " -- success");

        ADD_NATIVE_WORD(interpreter, "exit_failure", word_exit_failure,
            "Constant value for a process fail exit code.",
            " -- failure");

        ADD_NATIVE_WORD(interpreter, "none", word_none,
            "Push the value none onto the data stack.",
            " -- none");

        ADD_NATIVE_WORD(interpreter, "true", word_true,
            "Push the value true onto the data stack.",
            " -- true");

        ADD_NATIVE_WORD(interpreter, "false", word_false,
            "Push the value false onto the data stack.",
            " -- false");


        ADD_NATIVE_WORD(interpreter, "hex", word_hex,
            "Convert a number into a hex string.",
            "number -- hex_string");


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


        //ADD_NATIVE_WORD(interpreter, "show_bytecode", word_show_bytecode,
        //    "If set to true, show bytecode as it's generated.",
        //    "bool -- ");
        //
        //ADD_NATIVE_WORD(interpreter, "show_run_code", word_show_run_code,
        //    "If set to true show bytecode as it's executed.",
        //    "bool -- ");

        ADD_NATIVE_WORD(interpreter, "show-bytecode", word_show_word_bytecode,
            "Show detailed information about a word.",
            "word -- ");

        ADD_NATIVE_WORD(interpreter, "show-ir", word_show_ir,
            "Show the generated LLVM IR for the word.",
            "word -- ");

        ADD_NATIVE_WORD(interpreter, "show-asm", word_show_machine_code,
            "Show the generated machine code for the word.",
            "word -- ");

        ADD_NATIVE_WORD(interpreter, "sorth.execution-mode", word_sorth_execution_mode,
            "Get the current execution mode of the interpreter, either 'jit' or 'byte-code'.",
            " -- mode");

        PathPtr path = std::make_shared<std::filesystem::path>(__FILE__);
        register_word_info_struct(Location(path, __LINE__, 1), interpreter);
    }


}
