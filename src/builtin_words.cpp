
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "sorth.h"



namespace sorth
{


    using namespace internal;


    namespace
    {


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

                throw_error(interpreter->get_current_location(), stream.str());
            }
        }


        ArrayPtr as_array(InterpreterPtr& interpreter, const Value& value)
        {
            throw_error_if(!std::holds_alternative<ArrayPtr>(value),
                           interpreter->get_current_location(),
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

                throw_error(interpreter->get_current_location(), stream.str());
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
        interpreter->halt();
    }


    void word_reset(InterpreterPtr& interpreter)
    {
        interpreter->release_context();
        interpreter->mark_context();
    }


    void word_include(InterpreterPtr& interpreter)
    {
        auto path = as_string(interpreter, interpreter->pop());
        interpreter->process_source(std::filesystem::path(path));
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


    void word_op_mark_catch(InterpreterPtr& interpreter)
    {
        insert_user_instruction(interpreter,
            {
                .id = OperationCode::Id::mark_catch,
                .value = interpreter->pop()
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
                       interpreter->get_current_location(),
                       "Expected a byte code block.");

        interpreter->constructor()->stack.push({ .code = std::get<ByteCode>(top) });
    }


    void word_code_resolve_jumps(InterpreterPtr& interpreter)
    {
        auto is_jump = [](const OperationCode& code) -> bool
            {
                return    (code.id == OperationCode::Id::jump)
                       || (code.id == OperationCode::Id::jump_if_not_zero)
                       || (code.id == OperationCode::Id::jump_if_zero)
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

        interpreter->push(input_tokens[++current_token]);
    }


    void word_word_index(InterpreterPtr& interpreter)
    {
        auto& current_token = interpreter->constructor()->current_token;
        auto& input_tokens = interpreter->constructor()->input_tokens;

        ++current_token;
        auto name = input_tokens[current_token].text;

        interpreter->constructor()->stack.top().code.push_back({
                .id = OperationCode::Id::word_index,
                .value = name
            });
    }


    void word_execute(InterpreterPtr& interpreter)
    {
        auto index = as_numeric<int64_t>(interpreter, interpreter->pop());
        auto& handler = interpreter->get_handler_info(index);

        handler.function(interpreter);
    }


    void word_throw(InterpreterPtr& interpreter)
    {
        throw_error(interpreter->get_current_location(),
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
                .location = location,
                .is_immediate = false
            });

    }


    void word_end_word(InterpreterPtr& interpreter)
    {
        auto construction = interpreter->constructor()->stack.top();
        interpreter->constructor()->stack.pop();

        /*if (is_showing_bytecode)
        {
            std::cerr << "Defined word " << construction.name << std::endl
                      << construction.code << std::endl;
        }*/

        bool is_scripted = true;

        interpreter->add_word(construction.name,
                              ScriptWord(construction.name,
                                         construction.code,
                                         construction.location),
                              construction.location,
                              construction.is_immediate,
                              is_scripted);
    }


    void word_immediate(InterpreterPtr& interpreter)
    {
        interpreter->constructor()->stack.top().is_immediate = true;
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
            });

        for (int64_t i = 0; i < definition_ptr->fieldNames.size(); ++i)
        {
            ADD_NATIVE_WORD(interpreter, definition_ptr->name + "." + definition_ptr->fieldNames[i],
                [i](auto interpreter)
                {
                    interpreter->push(i);
                });
        }
    }


    void word_read_field(InterpreterPtr& interpreter)
    {
        auto var = interpreter->pop();

        throw_error_if(!std::holds_alternative<DataObjectPtr>(var),
                       interpreter->get_current_location(), "Expected data object.");

        auto object = std::get<DataObjectPtr>(var);

        var = interpreter->pop();
        auto field_index = as_numeric<int64_t>(interpreter, var);

        interpreter->push(object->fields[field_index]);
    }


    void word_write_field(InterpreterPtr& interpreter)
    {
        auto var = interpreter->pop();

        throw_error_if(!std::holds_alternative<DataObjectPtr>(var),
                       interpreter->get_current_location(), "Expected data object.");

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


    void word_and(InterpreterPtr& interpreter)
    {
        logic_bit_op(interpreter, [](auto a, auto b) { return a & b; });
    }


    void word_or(InterpreterPtr& interpreter)
    {
        logic_bit_op(interpreter, [](auto a, auto b) { return a | b; });
    }


    void word_xor(InterpreterPtr& interpreter)
    {
        logic_bit_op(interpreter, [](auto a, auto b) { return a ^ b; });
    }


    void word_not(InterpreterPtr& interpreter)
    {
        auto top = interpreter->pop();
        auto value = as_numeric<int64_t>(interpreter, top);

        value = ~value;

        interpreter->push(value);
    }


    void word_left_shift(InterpreterPtr& interpreter)
    {
        logic_bit_op(interpreter, [](auto value, auto amount) { return value << amount; });
    }


    void word_right_shift(InterpreterPtr& interpreter)
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


    void word_term_raw_mode(InterpreterPtr& interpreter)
    {
        // This part requires posix.  If we want to work on another os, this needs to be ported.
        static struct termios orig_termios;
        static bool is_in_raw_mode = false;

        auto requested_on = as_numeric<bool>(interpreter, interpreter->pop());

        if (requested_on && (!is_in_raw_mode))
        {
            struct termios raw = orig_termios;

            is_in_raw_mode = true;

            tcgetattr(STDIN_FILENO, &orig_termios);
            raw.c_lflag &= ~(ECHO | ICANON);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        }
        else if ((!requested_on) && is_in_raw_mode)
        {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        }
    }


    void word_print(InterpreterPtr& interpreter)
    {
        std::cout << interpreter->pop() << " ";
    }


    void word_flush(InterpreterPtr& interpreter)
    {
        std::cout << std::flush;
    }


    void word_print_nl(InterpreterPtr& interpreter)
    {
        std::cout << std::endl;
    }


    void word_hex(InterpreterPtr& interpreter)
    {
        auto int_value = as_numeric<int64_t>(interpreter, interpreter->pop());

        std::cout << std::hex << int_value << std::dec << " ";
    }


    void word_key(InterpreterPtr& interpreter)
    {
        char next[2] = { 0 };

        read(STDIN_FILENO, next, 1);
        interpreter->push(std::string(next));
    }


    void word_read_line(InterpreterPtr& interpreter)
    {
        std::string line;
        std::getline(std::cin, line);

        interpreter->push(line);
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
    }


    void word_show_run_code(InterpreterPtr& interpreter)
    {
    }


    void word_show_word(InterpreterPtr& interpreter)
    {
        auto& current_token = interpreter->constructor()->current_token;
        auto& input_tokens = interpreter->constructor()->input_tokens;

        ++current_token;

        auto name = input_tokens[current_token].text;
        auto [ found, word ] = interpreter->find_word(name);

        if (!found)
        {
            std::cerr << "Word, " << name << ", has not been defined." << std::endl;
            return;
        }

        auto& handler_info = interpreter->get_handler_info(word.handler_index);

        std::cout << "Word, " << word.handler_index << " -> " << name
                  << (word.is_scripted ? ", is user defined." : ", is a native word.")
                  << std::endl
                  << "Defined at: " << handler_info.definition_location
                  << "." << std::endl;

        if (word.is_immediate)
        {
            std::cout << "Word is immediate." << std::endl;
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
        ADD_NATIVE_WORD(interpreter, "quit", word_quit);
        ADD_NATIVE_WORD(interpreter, "reset", word_reset);
        ADD_NATIVE_WORD(interpreter, "include", word_include);

        // Words for creating new bytecode.
        ADD_NATIVE_WORD(interpreter, "op.def_variable", word_op_def_variable);
        ADD_NATIVE_WORD(interpreter, "op.def_constant", word_op_def_constant);
        ADD_NATIVE_WORD(interpreter, "op.read_variable", word_op_read_variable);
        ADD_NATIVE_WORD(interpreter, "op.write_variable", word_op_write_variable);
        ADD_NATIVE_WORD(interpreter, "op.execute", word_op_execute);
        ADD_NATIVE_WORD(interpreter, "op.push_constant_value", word_op_push_constant_value);
        ADD_NATIVE_WORD(interpreter, "op.mark_catch", word_op_mark_catch);
        ADD_NATIVE_WORD(interpreter, "op.jump", word_op_jump);
        ADD_NATIVE_WORD(interpreter, "op.jump_if_zero", word_op_jump_if_zero);
        ADD_NATIVE_WORD(interpreter, "op.jump_if_not_zero", word_op_jump_if_not_zero);
        ADD_NATIVE_WORD(interpreter, "op.jump_target", word_op_jump_target);

        ADD_NATIVE_WORD(interpreter, "code.new_block", word_code_new_block);
        ADD_NATIVE_WORD(interpreter, "code.merge_stack_block", word_code_merge_stack_block);
        ADD_NATIVE_WORD(interpreter, "code.pop_stack_block", word_code_pop_stack_block);
        ADD_NATIVE_WORD(interpreter, "code.push_stack_block", word_code_push_stack_block);
        ADD_NATIVE_WORD(interpreter, "code.resolve_jumps", word_code_resolve_jumps);
        ADD_NATIVE_WORD(interpreter, "code.compile_until_words", word_code_compile_until_words);
        ADD_NATIVE_WORD(interpreter, "code.insert_at_front", word_code_insert_at_front);

        ADD_NATIVE_WORD(interpreter, "code.execute_source", word_code_execute_source);

        ADD_NATIVE_WORD(interpreter, "word", word_word);
        ADD_IMMEDIATE_WORD(interpreter, "`", word_word_index);
        ADD_NATIVE_WORD(interpreter, "execute", word_execute);

        ADD_NATIVE_WORD(interpreter, "throw", word_throw);

        // Creating new words.
        ADD_IMMEDIATE_WORD(interpreter, ":", word_start_word);
        ADD_IMMEDIATE_WORD(interpreter, ";", word_end_word);
        ADD_IMMEDIATE_WORD(interpreter, "immediate", word_immediate);

        // Data structure support.
        ADD_IMMEDIATE_WORD(interpreter, "#", word_data_definition);
        ADD_NATIVE_WORD(interpreter, "#@", word_read_field);
        ADD_NATIVE_WORD(interpreter, "#!", word_write_field);

        // Array and string words.

        ADD_NATIVE_WORD(interpreter, "[].new", word_array_new);
        ADD_NATIVE_WORD(interpreter, "[].size", word_array_size);
        ADD_NATIVE_WORD(interpreter, "[]!", word_array_write_index);
        ADD_NATIVE_WORD(interpreter, "[]@", word_array_read_index);
        ADD_NATIVE_WORD(interpreter, "[].resize", word_array_resize);

        // ByteBuffer operations.
        ADD_NATIVE_WORD(interpreter, "buffer.new", word_buffer_new);

        ADD_NATIVE_WORD(interpreter, "buffer.int!", word_buffer_write_int);
        ADD_NATIVE_WORD(interpreter, "buffer.int@", word_buffer_read_int);

        ADD_NATIVE_WORD(interpreter, "buffer.float!", word_buffer_write_float);
        ADD_NATIVE_WORD(interpreter, "buffer.float@", word_buffer_read_float);

        ADD_NATIVE_WORD(interpreter, "buffer.string!", word_buffer_write_string);
        ADD_NATIVE_WORD(interpreter, "buffer.string@", word_buffer_read_string);

        ADD_NATIVE_WORD(interpreter, "buffer.position!", word_buffer_set_postion);
        ADD_NATIVE_WORD(interpreter, "buffer.position@", word_buffer_get_postion);

        // Math ops.
        ADD_NATIVE_WORD(interpreter, "+", word_add);
        ADD_NATIVE_WORD(interpreter, "-", word_subtract);
        ADD_NATIVE_WORD(interpreter, "*", word_multiply);
        ADD_NATIVE_WORD(interpreter, "/", word_divide);

        // Bitwise operator words.
        ADD_NATIVE_WORD(interpreter, "and", word_and);
        ADD_NATIVE_WORD(interpreter, "or", word_or);
        ADD_NATIVE_WORD(interpreter, "xor", word_xor);
        ADD_NATIVE_WORD(interpreter, "not", word_not);
        ADD_NATIVE_WORD(interpreter, "<<", word_left_shift);
        ADD_NATIVE_WORD(interpreter, ">>", word_right_shift);

        // Equality words.
        ADD_NATIVE_WORD(interpreter, "=", word_equal);
        ADD_NATIVE_WORD(interpreter, "<>", word_not_equal);
        ADD_NATIVE_WORD(interpreter, ">=", word_greater_equal);
        ADD_NATIVE_WORD(interpreter, "<=", word_less_equal);
        ADD_NATIVE_WORD(interpreter, ">", word_greater);
        ADD_NATIVE_WORD(interpreter, "<", word_less);

        // Stack words.
        ADD_NATIVE_WORD(interpreter, "dup", word_dup);
        ADD_NATIVE_WORD(interpreter, "drop", word_drop);
        ADD_NATIVE_WORD(interpreter, "swap", word_swap);
        ADD_NATIVE_WORD(interpreter, "over", word_over);
        ADD_NATIVE_WORD(interpreter, "rot", word_rot);

        // Some built in constants.
        ADD_NATIVE_WORD(interpreter, "unique_str", word_unique_str);

        ADD_NATIVE_WORD(interpreter, "exit_success", word_exit_success);
        ADD_NATIVE_WORD(interpreter, "exit_failure", word_exit_failure);
        ADD_NATIVE_WORD(interpreter, "true", word_true);
        ADD_NATIVE_WORD(interpreter, "false", word_false);

        // Debug, printing, input, and terminal control words.
        ADD_NATIVE_WORD(interpreter, "term.raw_mode", word_term_raw_mode);

        ADD_NATIVE_WORD(interpreter, ".", word_print);
        ADD_NATIVE_WORD(interpreter, "flush", word_flush);
        ADD_NATIVE_WORD(interpreter, "cr", word_print_nl);
        ADD_NATIVE_WORD(interpreter, ".hex", word_hex);

        ADD_NATIVE_WORD(interpreter, "key", word_key);
        ADD_NATIVE_WORD(interpreter, "readline", word_read_line);

        ADD_NATIVE_WORD(interpreter, ".s", word_print_stack);
        ADD_NATIVE_WORD(interpreter, ".w", word_print_dictionary);

        ADD_NATIVE_WORD(interpreter, "show_bytecode", word_show_bytecode);
        ADD_NATIVE_WORD(interpreter, "show_run_code", word_show_run_code);
        ADD_IMMEDIATE_WORD(interpreter, "show_word", word_show_word);
    }


}
