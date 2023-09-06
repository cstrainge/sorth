
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <cstdlib>

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
                            interpreter->release_context();
                        }
                    };

            private:
                std::string name;
                ByteCode code;

            public:
                ScriptWord(const std::string& new_name, const ByteCode& new_code,
                           const Location& new_location)
                : name(new_name),
                  code(new_code)
                {
                }

            public:
                void operator ()(InterpreterPtr& interpreter)
                {
                    ContextManager manager(interpreter);
                    interpreter->execute_code(name, code);
                }

            public:
                void show(std::ostream& stream, InterpreterPtr& interpreter,
                          const std::vector<std::string>& inverse_list)
                {
                    for (size_t i = 0; i < code.size(); ++i)
                    {
                        stream << std::setw(6) << i << "  ";

                        if (   (code[i].id == OperationCode::Id::execute)
                            && (is_numeric(code[i].value)))
                        {
                            auto index = as_numeric<int64_t>(interpreter, code[i].value);

                            stream << code[i].id
                                << "  " << index;


                            if (inverse_list[index] != "")
                            {
                                stream << " -> " << inverse_list[index];
                            }

                            stream << std::endl;
                        }
                        else
                        {
                            stream << code[i] << std::endl;
                        }
                    }
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

                throw_error(*interpreter, stream.str());
            }
        }


        ArrayPtr as_array(InterpreterPtr& interpreter, const Value& value)
        {
            throw_error_if(!std::holds_alternative<ArrayPtr>(value),
                           *interpreter,
                           "Expected an array object.");

            return std::get<ArrayPtr>(value);
        }


        void check_buffer_index(InterpreterPtr& interpreter,
                                const ByteBufferPtr& buffer, int64_t byte_size)
        {
            if ((buffer->postion() + byte_size) > buffer->size())
            {
                std::stringstream stream;

                stream << "Writing a value of size " << byte_size << " at a position of "
                       << buffer->postion() << " would exceed the buffer size, "
                       << buffer->size() << ".";

                throw_error(*interpreter, stream.str());
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
                                  std::function<void(double,double)> dop,
                                  std::function<void(int64_t,int64_t)> iop,
                                  std::function<void(std::string,std::string)> sop)
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


    }


    void word_quit(InterpreterPtr& interpreter)
    {
        if (!interpreter->is_stack_empty())
        {
            auto value = interpreter->pop();

            if (is_numeric(value))
            {
                auto exit_code = as_numeric<int64_t>(interpreter, value);
                interpreter->set_exit_code(exit_code);
            }
        }

        interpreter->halt();
    }


    void word_reset(InterpreterPtr& interpreter)
    {
        // Once the reset occurs any handler registered has probably ceased to exist.  So,
        // unregister it now.

        at_exit_interpreter = nullptr;
        at_exit_value = 0;

        interpreter->release_context();
        interpreter->mark_context();
    }


    void word_at_exit(InterpreterPtr& interpreter)
    {
        at_exit_interpreter = interpreter;
        at_exit_value = interpreter->pop();

        atexit([]()
            {
                try
                {
                    if (at_exit_interpreter)
                    {
                        // Make sure that the halt flag is cleared so the interpreter will run our
                        // code.
                        at_exit_interpreter->clear_halt_flag();

                        // Now attempt to execute the at_exit word handler.
                        if (is_string(at_exit_value))
                        {
                            auto word_name = as_string(at_exit_interpreter, at_exit_value);
                            at_exit_interpreter->execute_word(word_name);
                        }
                        else
                        {
                            auto word_index = as_numeric<int64_t>(at_exit_interpreter,
                                                                  at_exit_value);
                            auto& word_info = at_exit_interpreter->get_handler_info(word_index);

                            word_info.function(at_exit_interpreter);
                        }
                    }
                }
                catch (const std::runtime_error& error)
                {
                    std::cerr << "Exit handler exception: " << error.what() << std::endl;
                }
                catch (...)
                {
                    std::cerr << "Exit handler: Unexpected exception occurred." << std::endl;
                }

                at_exit_interpreter = nullptr;
            });
    }


    void word_include(InterpreterPtr& interpreter)
    {
        auto path = as_string(interpreter, interpreter->pop());
        interpreter->process_source(interpreter->find_file(path));
    }


    void insert_user_instruction(InterpreterPtr& interpreter, const OperationCode& op)
    {
        auto& constructor = interpreter->constructor();
        auto& code = constructor->stack.top().code;

        if (!constructor->user_is_inserting_at_beginning)
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
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::def_variable,
                .value = interpreter->pop()
            });
    }


    void word_op_def_constant(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::def_constant,
                .value = interpreter->pop()
            });
    }


    void word_op_read_variable(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::read_variable,
                .value = 0
            });
    }


    void word_op_write_variable(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::write_variable,
                .value = 0
            });
    }


    void word_op_execute(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::execute,
                .value = interpreter->pop()
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
                .value = 0
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
                .value = 0
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
                .value = 0
            });
    }


    void word_jump_loop_exit(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::jump_loop_exit,
                .value = 0
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
        interpreter->constructor()->stack.push({});
    }


    void word_code_merge_stack_block(InterpreterPtr& interpreter)
    {
        auto& stack = interpreter->constructor()->stack;
        auto top_code = stack.top().code;

        stack.pop();

        stack.top().code.insert(stack.top().code.end(), top_code.begin(), top_code.end());
    }


    void word_code_pop_stack_block(InterpreterPtr& interpreter)
    {
        interpreter->push(interpreter->constructor()->stack.top().code);
        interpreter->constructor()->stack.pop();
    }


    void word_code_push_stack_block(InterpreterPtr& interpreter)
    {
        auto top = interpreter->pop();

        throw_error_if(!std::holds_alternative<ByteCode>(top),
                       *interpreter,
                       "Expected a byte code block.");

        interpreter->constructor()->stack.push({ .code = std::get<ByteCode>(top) });
    }


    void word_code_stack_block_size(InterpreterPtr& interpreter)
    {
        interpreter->push((int64_t)interpreter->constructor()->stack.top().code.size());
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

        auto& top_code = interpreter->constructor()->stack.top().code;

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
                top_code[i].value = 0;
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

        auto& current_token = interpreter->constructor()->current_token;
        auto& input_tokens = interpreter->constructor()->input_tokens;

        auto start_location = input_tokens[current_token - 1].location;

        for (++current_token; current_token < input_tokens.size(); ++current_token)
        {
            auto& token = input_tokens[current_token];
            auto [found, word] = is_one_of_words(token.text);

            if ((found) && (token.type == Token::Type::word))
            {
                interpreter->push(word);
                return;
            }

            interpreter->constructor()->compile_token(token);
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
        interpreter->constructor()->user_is_inserting_at_beginning =
                                                  as_numeric<bool>(interpreter, interpreter->pop());
    }


    void word_code_execute_source(InterpreterPtr& interpreter)
    {
        interpreter->process_source(as_string(interpreter, interpreter->pop()));
    }


    void word_word(InterpreterPtr& interpreter)
    {
        auto& current_token = interpreter->constructor()->current_token;
        auto& input_tokens = interpreter->constructor()->input_tokens;

        interpreter->push(input_tokens[++current_token].text);
    }


    void word_word_index(InterpreterPtr& interpreter)
    {
        auto& current_token = interpreter->constructor()->current_token;
        auto& input_tokens = interpreter->constructor()->input_tokens;

        ++current_token;
        auto name = input_tokens[current_token].text;

        auto [ found, word ] = interpreter->find_word(name);

        if (found)
        {
            interpreter->constructor()->stack.top().code.push_back({
                    .id = OperationCode::Id::push_constant_value,
                    .value = (int64_t)word.handler_index
                });
        }
        else
        {
            interpreter->constructor()->stack.top().code.push_back({
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
            throw_error(*interpreter, "Unexpected value type for execute.");
        }
    }


    void word_is_defined(InterpreterPtr& interpreter)
    {
        auto& current_token = interpreter->constructor()->current_token;
        auto& input_tokens = interpreter->constructor()->input_tokens;

        ++current_token;
        auto name = input_tokens[current_token].text;

        interpreter->constructor()->stack.top().code.push_back({
                .id = OperationCode::Id::word_exists,
                .value = name
            });
    }


    void word_throw(InterpreterPtr& interpreter)
    {
        throw_error(*interpreter,
                    as_string(interpreter, interpreter->pop()));
    }


    void word_start_word(InterpreterPtr& interpreter)
    {
        auto& current_token = interpreter->constructor()->current_token;
        auto& input_tokens = interpreter->constructor()->input_tokens;

        ++current_token;
        auto& name = input_tokens[current_token].text;
        auto& location = input_tokens[current_token].location;

        interpreter->constructor()->stack.push({
                .name = name,
                .description = "",
                .location = location,
                .is_immediate = false,
                .is_hidden = false
            });

    }


    void word_end_word(InterpreterPtr& interpreter)
    {
        auto construction = interpreter->constructor()->stack.top();
        interpreter->constructor()->stack.pop();

        auto new_word = ScriptWord(construction.name, construction.code, construction.location);

        if (interpreter->showing_bytecode())
        {
            std::cout << "--------[" << construction.name << "]-------------" << std::endl;
            auto inverse_list = interpreter->get_inverse_lookup_list();
            new_word.show(std::cout, interpreter, inverse_list);
        }

        bool is_scripted = true;

        interpreter->add_word(construction.name,
                              new_word,
                              construction.location,
                              construction.is_immediate,
                              construction.is_hidden,
                              is_scripted,
                              construction.description);
    }


    void word_immediate(InterpreterPtr& interpreter)
    {
        interpreter->constructor()->stack.top().is_immediate = true;
    }


    void word_hidden(InterpreterPtr& interpreter)
    {
        interpreter->constructor()->stack.top().is_hidden = true;
    }


    void word_description(InterpreterPtr& interpreter)
    {
        auto& current_token = interpreter->constructor()->current_token;
        auto& input_tokens = interpreter->constructor()->input_tokens;

        ++current_token;

        auto& token = input_tokens[current_token];

        throw_error_if(token.type != Token::Type::string,
                       *interpreter,
                       "Expected the description to be a string.");

        interpreter->constructor()->stack.top().description = token.text;
    }


    void word_is_value_number(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(   std::holds_alternative<int64_t>(value)
                          || std::holds_alternative<double>(value));
    }


    void word_is_value_boolean(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<bool>(value));
    }


    void word_is_value_string(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<std::string>(value));
    }


    void word_is_value_structure(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<DataObjectPtr>(value));
    }


    void word_is_value_array(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<ArrayPtr>(value));
    }


    void word_is_value_buffer(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<ByteBufferPtr>(value));
    }


    void word_is_value_hash_table(InterpreterPtr& interpreter)
    {
        auto value = interpreter->pop();

        interpreter->push(std::holds_alternative<HashTablePtr>(value));
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
        auto position = (size_t)as_numeric<int64_t>(interpreter, interpreter->pop());
        auto count = (size_t)as_numeric<int64_t>(interpreter, interpreter->pop());

        string.erase(position, count);

        interpreter->push(string);
    }


    void word_string_find(InterpreterPtr& interpreter)
    {
        auto string = as_string(interpreter, interpreter->pop());
        auto search_str = as_string(interpreter, interpreter->pop());

        interpreter->push((int64_t)string.find(search_str, 0));
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
            interpreter->push(std::strtoll(string.c_str(), nullptr, 10));
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
        auto& current_token = interpreter->constructor()->current_token;
        auto& input_tokens = interpreter->constructor()->input_tokens;

        DataObjectDefinition definition;

        ++current_token;
        DataObjectDefinitionPtr definition_ptr = std::make_shared<DataObjectDefinition>();
        definition_ptr->name = input_tokens[current_token].text;

        for (++current_token; current_token < input_tokens.size(); ++current_token)
        {
            if (input_tokens[current_token].text == ";")
            {
                break;
            }

            definition_ptr->fieldNames.push_back(input_tokens[current_token].text);
        }

        ADD_NATIVE_WORD(interpreter, definition_ptr->name + ".new",
            [definition_ptr](auto interpreter)
            {
                DataObjectPtr new_object = std::make_shared<DataObject>();

                new_object->definition = definition_ptr;
                new_object->fields.resize(definition_ptr->fieldNames.size());

                interpreter->push(new_object);
            },
            "Create a new instance of the structure " + definition_ptr->name + ".");

        for (int64_t i = 0; i < definition_ptr->fieldNames.size(); ++i)
        {
            ADD_NATIVE_WORD(interpreter, definition_ptr->name + "." + definition_ptr->fieldNames[i],
                [i](auto interpreter)
                {
                    interpreter->push(i);
                },
                "Access the structure field + " + definition_ptr->fieldNames[i] + ".");
        }
    }


    void word_read_field(InterpreterPtr& interpreter)
    {
        auto var = interpreter->pop();

        throw_error_if(!std::holds_alternative<DataObjectPtr>(var),
                       *interpreter, "Expected data object.");

        auto object = std::get<DataObjectPtr>(var);

        var = interpreter->pop();
        auto field_index = as_numeric<int64_t>(interpreter, var);

        interpreter->push(object->fields[field_index]);
    }


    void word_write_field(InterpreterPtr& interpreter)
    {
        auto var = interpreter->pop();

        throw_error_if(!std::holds_alternative<DataObjectPtr>(var),
                       *interpreter, "Expected data object.");

        auto object = std::get<DataObjectPtr>(var);

        var = interpreter->pop();
        auto field_index = as_numeric<int64_t>(interpreter, var);

        var = interpreter->pop();
        object->fields[field_index] = var;
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


    void word_array_resize(InterpreterPtr& interpreter)
    {
        auto array = as_array(interpreter, interpreter->pop());
        auto new_size = as_numeric<int64_t>(interpreter, interpreter->pop());

        array->resize(new_size);
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


    void word_buffer_set_postion(InterpreterPtr& interpreter)
    {
        auto buffer = as_byte_buffer(interpreter, interpreter->pop());
        auto new_position = as_numeric<int64_t>(interpreter, interpreter->pop());

        buffer->set_position(new_position);
    }


    void word_buffer_get_postion(InterpreterPtr& interpreter)
    {
        auto buffer = as_byte_buffer(interpreter, interpreter->pop());

        interpreter->push(buffer->postion());
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

        throw_error_if (!found, *interpreter, "Value does not exist in table.");

        interpreter->push(value);
    }


    void word_hash_table_exists(InterpreterPtr& interpreter)
    {
        auto table = as_hash_table(interpreter, interpreter->pop());
        auto key = interpreter->pop();

        auto [ found, value ] = table->get(key);

        interpreter->push(found);
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


    void word_not_equal(InterpreterPtr& interpreter)
    {
        string_or_numeric_op(interpreter,
                             [&](auto a, auto b) { interpreter->push((bool)(a != b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a != b)); },
                             [&](auto a, auto b) { interpreter->push((bool)(a != b)); });
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


    void word_unique_str(InterpreterPtr& interpreter)
    {
        static int32_t index = 0;

        std::stringstream stream;
        stream << "unique-" << std::setw(4) << std::setfill('0') << std::hex << index;
        ++index;

        interpreter->push(stream.str());
    }


    void word_exit_success(InterpreterPtr& interpreter)
    {
        interpreter->push(EXIT_SUCCESS);
    }


    void word_exit_failure(InterpreterPtr& interpreter)
    {
        interpreter->push(EXIT_FAILURE);
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


    void word_show_bytecode(InterpreterPtr& interpreter)
    {
        interpreter->showing_bytecode() = as_numeric<bool>(interpreter, interpreter->pop());
    }


    void word_show_run_code(InterpreterPtr& interpreter)
    {
        interpreter->showing_run_code() = as_numeric<bool>(interpreter, interpreter->pop());
    }


    void word_show_word(InterpreterPtr& interpreter)
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
            std::cerr << "Word, " << name << ", has not been defined." << std::endl;
            return;
        }

        auto& handler_info = interpreter->get_handler_info(word.handler_index);

        std::cout << "Word, " << word.handler_index << " -> " << name
                  << (word.is_scripted ? "" : ", is a native word.")
                  << std::endl
                  << "Defined at: " << handler_info.definition_location
                  << "." << std::endl;

        if (word.is_immediate)
        {
            std::cout << "Word is immediate." << std::endl;
        }

        if (word.is_hidden)
        {
            std::cout << "Word is hidden from the directory." << std::endl;
        }

        if (word.description)
        {
            std::cout << "Description: " << (*word.description) << std::endl;
        }

        if (word.is_scripted)
        {
            auto& handler = handler_info.function;
            auto script_handler = handler.target<ScriptWord>();
            auto inverse_list = interpreter->get_inverse_lookup_list();

            script_handler->show(std::cout, interpreter, inverse_list);
        }
    }


    void register_builtin_words(InterpreterPtr& interpreter)
    {
        // Manage the interpreter state.
        ADD_NATIVE_WORD(interpreter, "quit", word_quit,
                        "Exit the interpreter.");

        ADD_NATIVE_WORD(interpreter, "reset", word_reset,
                        "Reset the interpreter to it's default state.");

        ADD_NATIVE_WORD(interpreter, "at_exit", word_at_exit,
                        "Run the given word when the interpreter exits.");

        ADD_NATIVE_WORD(interpreter, "include", word_include,
                        "Include and execute another source file.");


        // Words for creating new bytecode.
        ADD_NATIVE_WORD(interpreter, "op.def_variable", word_op_def_variable,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.def_constant", word_op_def_constant,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.read_variable", word_op_read_variable,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.write_variable", word_op_write_variable,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.execute", word_op_execute,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.push_constant_value", word_op_push_constant_value,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.mark_loop_exit", word_mark_loop_exit,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.unmark_loop_exit", word_unmark_loop_exit,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.mark_catch", word_op_mark_catch,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.unmark_catch", word_op_unmark_catch,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.jump", word_op_jump,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.jump_if_zero", word_op_jump_if_zero,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.jump_if_not_zero", word_op_jump_if_not_zero,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.jump_loop_start", word_jump_loop_start,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.jump_loop_exit", word_jump_loop_exit,
                        "Insert this instruction into the byte stream.");

        ADD_NATIVE_WORD(interpreter, "op.jump_target", word_op_jump_target,
                        "Insert this instruction into the byte stream.");


        ADD_NATIVE_WORD(interpreter, "code.new_block", word_code_new_block,
                        "Create a new sub-block on the code generation stack.");

        ADD_NATIVE_WORD(interpreter, "code.merge_stack_block", word_code_merge_stack_block,
                        "Merge the top code block into the one below.");

        ADD_NATIVE_WORD(interpreter, "code.pop_stack_block", word_code_pop_stack_block,
                        "Pop a code block off of the code stack and onto the data stack.");

        ADD_NATIVE_WORD(interpreter, "code.push_stack_block", word_code_push_stack_block,
                        "Pop a block from the data stack and back onto the code stack.");

        ADD_NATIVE_WORD(interpreter, "code.stack_block_size@", word_code_stack_block_size,
                        "Read the size of the code block at the top of the stack.");

        ADD_NATIVE_WORD(interpreter, "code.resolve_jumps", word_code_resolve_jumps,
                        "Resolve all of the jumps in the top code block.");

        ADD_NATIVE_WORD(interpreter, "code.compile_until_words", word_code_compile_until_words,
                        "Compile words until one of the given words is found.");

        ADD_NATIVE_WORD(interpreter, "code.insert_at_front", word_code_insert_at_front,
                        "When true new instructions are added beginning of the block.");


        ADD_NATIVE_WORD(interpreter, "code.execute_source", word_code_execute_source,
                        "Interpret and execute a string like it is source code.");


        // Word words.
        ADD_NATIVE_WORD(interpreter, "word", word_word,
                        "Get the next word in the token stream.");

        ADD_IMMEDIATE_WORD(interpreter, "`", word_word_index,
                           "Get the index of the next word.");

        ADD_NATIVE_WORD(interpreter, "execute", word_execute,
                        "Execute a word name or index.");

        ADD_IMMEDIATE_WORD(interpreter, "defined?", word_is_defined,
                           "Is the given word defined?");


        // Exception time
        ADD_NATIVE_WORD(interpreter, "throw", word_throw,
                        "Throw an exception with the given message.");


        // Creating new words.
        ADD_IMMEDIATE_WORD(interpreter, ":", word_start_word,
                           "The start of a new word definition.");

        ADD_IMMEDIATE_WORD(interpreter, ";", word_end_word,
                           "The end of a new word definition.");

        ADD_IMMEDIATE_WORD(interpreter, "immediate", word_immediate,
                           "Mark the current word being built as immediate.");

        ADD_IMMEDIATE_WORD(interpreter, "hidden", word_hidden,
                           "Mark the current word being built as hidden.");

        ADD_IMMEDIATE_WORD(interpreter, "description:", word_description,
                           "Give a new word it's description.");


        // Check value types.
        ADD_NATIVE_WORD(interpreter, "is_value_number?", word_is_value_number,
                        "Is the value a number?");

        ADD_NATIVE_WORD(interpreter, "is_value_boolean?", word_is_value_boolean,
                        "Is the value a boolean?");

        ADD_NATIVE_WORD(interpreter, "is_value_string?", word_is_value_string,
                        "Is the value a string?");

        ADD_NATIVE_WORD(interpreter, "is_value_structure?", word_is_value_structure,
                        "Is the value a structure?");

        ADD_NATIVE_WORD(interpreter, "is_value_array?", word_is_value_array,
                        "Is the value an array?");

        ADD_NATIVE_WORD(interpreter, "is_value_buffer?", word_is_value_buffer,
                        "Is the value a byte buffer?");

        ADD_NATIVE_WORD(interpreter, "is_value_hash_table?", word_is_value_hash_table,
                        "Is the value a hash table?");


        // String words.
        ADD_NATIVE_WORD(interpreter, "string.length", word_string_length,
                        "Get the length of a given string.");

        ADD_NATIVE_WORD(interpreter, "string.insert", word_string_insert,
                        "Insert a string into another string.");

        ADD_NATIVE_WORD(interpreter, "string.remove", word_string_remove,
                        "Remove some characters from a string.");

        ADD_NATIVE_WORD(interpreter, "string.find", word_string_find,
                        "Find the first instance of a string within another.");

        ADD_NATIVE_WORD(interpreter, "string.to_number", word_string_to_number,
                        "Convert a string into a number.");

        ADD_NATIVE_WORD(interpreter, "to_string", word_to_string,
                        "Convert a value to a string.");

        ADD_NATIVE_WORD(interpreter, "string.npos", [](auto& interpreter)
            {
                interpreter->push((int64_t)std::string::npos);
            },
            "Constant value that indicates a search has failed.");


        // Data structure support.
        ADD_IMMEDIATE_WORD(interpreter, "#", word_data_definition,
                           "Beginning of a structure definition.");

        ADD_NATIVE_WORD(interpreter, "#@", word_read_field,
                        "Read a field from a structure.");

        ADD_NATIVE_WORD(interpreter, "#!", word_write_field,
                        "Write to a field of a structure.");


        // Array words.

        ADD_NATIVE_WORD(interpreter, "[].new", word_array_new,
                        "Create a new array with the given default size.");

        ADD_NATIVE_WORD(interpreter, "[].size@", word_array_size,
                        "Read the size of the array object.");

        ADD_NATIVE_WORD(interpreter, "[]!", word_array_write_index,
                        "Write to a value in the array.");

        ADD_NATIVE_WORD(interpreter, "[]@", word_array_read_index,
                        "Read a value from the array.");

        ADD_NATIVE_WORD(interpreter, "[].size!", word_array_resize,
                        "Grow or shrink the array to the new size.");


        // ByteBuffer operations.
        ADD_NATIVE_WORD(interpreter, "buffer.new", word_buffer_new,
                        "Create a new byte buffer.");


        ADD_NATIVE_WORD(interpreter, "buffer.int!", word_buffer_write_int,
                        "Write an integer of a given size to the buffer.");

        ADD_NATIVE_WORD(interpreter, "buffer.int@", word_buffer_read_int,
                        "Read an integer of a given size from the buffer.");


        ADD_NATIVE_WORD(interpreter, "buffer.float!", word_buffer_write_float,
                        "Write a float of a given size to the buffer.");

        ADD_NATIVE_WORD(interpreter, "buffer.float@", word_buffer_read_float,
                        "read a float of a given size from the buffer.");


        ADD_NATIVE_WORD(interpreter, "buffer.string!", word_buffer_write_string,
                        "Write a string of given size to the buffer.  Padded with 0s if needed.");

        ADD_NATIVE_WORD(interpreter, "buffer.string@", word_buffer_read_string,
                        "Read a string of a given max size from the buffer.");


        ADD_NATIVE_WORD(interpreter, "buffer.position!", word_buffer_set_postion,
                        "Set the position of the buffer pointer.");

        ADD_NATIVE_WORD(interpreter, "buffer.position@", word_buffer_get_postion,
                        "Get the position of the buffer pointer.");


        // HashTable operations.
        ADD_NATIVE_WORD(interpreter, "{}.new", word_hash_table_new,
                        "Create a new hash table.");

        ADD_NATIVE_WORD(interpreter, "{}!", word_hash_table_insert,
                        "Write a value to a given key in the table.");

        ADD_NATIVE_WORD(interpreter, "{}@", word_hash_table_find,
                        "Read a value from a given key in the table.");

        ADD_NATIVE_WORD(interpreter, "{}?", word_hash_table_exists,
                        "Check if a given key exists in the table.");

        ADD_NATIVE_WORD(interpreter, "{}.iterate", word_hash_table_iterate,
                        "Iterate through a hash table and call a word for each item.");


        // Math ops.
        ADD_NATIVE_WORD(interpreter, "+", word_add,
                        "Add 2 numbers or strings together.");

        ADD_NATIVE_WORD(interpreter, "-", word_subtract,
                        "Subtract 2 numbers.");

        ADD_NATIVE_WORD(interpreter, "*", word_multiply,
                        "Multiply 2 numbers.");

        ADD_NATIVE_WORD(interpreter, "/", word_divide,
                        "Divide 2 numbers.");


        // Logical words.
        ADD_NATIVE_WORD(interpreter, "&&", word_logic_and,
                        "Logically compare 2 values.");

        ADD_NATIVE_WORD(interpreter, "||", word_logic_or,
                        "Logically compare 2 values.");

        ADD_NATIVE_WORD(interpreter, "'", word_logic_not,
                        "Logically invert a boolean value.");


        // Bitwise operator words.
        ADD_NATIVE_WORD(interpreter, "&", word_bit_and,
                        "Bitwise AND two numbers together.");

        ADD_NATIVE_WORD(interpreter, "|", word_bit_or,
                        "Bitwise OR two numbers together.");

        ADD_NATIVE_WORD(interpreter, "^", word_bit_xor,
                        "Bitwise XOR two numbers together.");

        ADD_NATIVE_WORD(interpreter, "~", word_bit_not,
                        "Bitwise NOT a number.");

        ADD_NATIVE_WORD(interpreter, "<<", word_bit_left_shift,
                        "Shift a numbers bits to the left.");

        ADD_NATIVE_WORD(interpreter, ">>", word_bit_right_shift,
                        "Shift a numbers bits to the right.");


        // Equality words.
        ADD_NATIVE_WORD(interpreter, "=", word_equal,
                        "Are 2 values equal?");

        ADD_NATIVE_WORD(interpreter, "<>", word_not_equal,
                        "Are 2 values not equal?");

        ADD_NATIVE_WORD(interpreter, ">=", word_greater_equal,
                        "Is one value greater or equal to another?");

        ADD_NATIVE_WORD(interpreter, "<=", word_less_equal,
                        "Is one value less than or equal to another?");

        ADD_NATIVE_WORD(interpreter, ">", word_greater,
                        "Is one value greater than another?");

        ADD_NATIVE_WORD(interpreter, "<", word_less,
                        "Is one value less than another?");


        // Stack words.
        ADD_NATIVE_WORD(interpreter, "dup", word_dup,
                        "Duplicate the top value on the data stack.");

        ADD_NATIVE_WORD(interpreter, "drop", word_drop,
                        "Discard the top value on the data stack.");

        ADD_NATIVE_WORD(interpreter, "swap", word_swap,
                        "Swap the top 2 values on the data stack.");

        ADD_NATIVE_WORD(interpreter, "over", word_over,
                        "Make a copy of the top value and place the copy under the second.");

        ADD_NATIVE_WORD(interpreter, "rot", word_rot,
                        "Rotate the top 3 values on the stack.");


        // Some built in constants.
        ADD_NATIVE_WORD(interpreter, "unique_str", word_unique_str,
                        "Generate a unique string and push it onto the data stack.");


        ADD_NATIVE_WORD(interpreter, "exit_success", word_exit_success,
                        "Constant value for a process success exit code.");

        ADD_NATIVE_WORD(interpreter, "exit_failure", word_exit_failure,
                        "Constant value for a process fail exit code.");

        ADD_NATIVE_WORD(interpreter, "true", word_true,
                        "Push the value true onto the data stack.");

        ADD_NATIVE_WORD(interpreter, "false", word_false,
                        "Push the value false onto the data stack.");


        ADD_NATIVE_WORD(interpreter, "hex", word_hex,
                        "Convert a number into a hex string.");


        ADD_NATIVE_WORD(interpreter, ".s", word_print_stack,
                        "Print out the data stack without changing it.");

        ADD_NATIVE_WORD(interpreter, ".w", word_print_dictionary,
                        "Print out the current word dictionary.");


        ADD_NATIVE_WORD(interpreter, "show_bytecode", word_show_bytecode,
                        "If set to true, show bytecode as it's generated.");

        ADD_NATIVE_WORD(interpreter, "show_run_code", word_show_run_code,
                        "If set to true show bytecode as it's executed.");

        ADD_NATIVE_WORD(interpreter, "show_word", word_show_word,
                        "Show detailed information about a word.");

    }


}
