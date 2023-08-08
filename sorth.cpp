
#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <stack>
#include <string>
#include <vector>
#include <variant>
#include <unordered_map>



namespace
{


    class Location
    {
        private:
            size_t line;
            size_t column;

        public:
            Location()
            : line(1),
              column(1)
            {
            }

        public:
            size_t get_line() const noexcept
            {
                return line;
            }

            size_t get_column() const noexcept
            {
                return column;
            }

        public:
            void next() noexcept
            {
                ++column;
            }

            void next_line() noexcept
            {
                ++line;
                column = 1;
            }
    };


    std::ostream& operator <<(std::ostream& stream, Location location)
    {
        stream << "(" << location.get_line() << ", " << location.get_column() << ")";

        return stream;
    }


    class SourceBuffer
    {
        private:
            std::string source;
            size_t position;
            Location source_location;

        public:
            SourceBuffer()
            : source(),
              position(0),
              source_location()
            {
            }

            SourceBuffer(std::string const& new_source)
            : source(new_source),
              position(0),
              source_location()
            {
            }

        public:
            operator bool() const noexcept
            {
                return position < source.size();
            }

        public:
            char peek_next() const noexcept
            {
                if (position >= source.size())
                {
                    return ' ';
                }

                return source[position];
            }

            char next() noexcept
            {
                auto next = peek_next();

                if (*this)
                {
                    increment_position(next);
                }

                return next;
            }

            Location current_location() const noexcept
            {
                return source_location;
            }

        private:
            void increment_position(char next) noexcept
            {
                ++position;

                if (next == '\n')
                {
                    source_location.next_line();
                }
                else
                {
                    source_location.next();
                }
            }
    };


    struct Token
    {
        enum class Type
        {
            number,
            string,
            word
        };

        Type type;
        Location location;
        std::string text;
    };


    using TokenList = std::vector<Token>;


    TokenList tokenize(const std::string& source_text)
    {
        TokenList tokens;
        SourceBuffer source(source_text);

        auto get_while = [&](std::function<bool(char)> test) -> std::string
            {
                std::string new_string;
                char next = source.peek_next();

                while (    (source)
                        && (test(next)))
                {
                    new_string += source.next();
                    next = source.peek_next();
                }

                return new_string;
            };

        auto is_whitespace = [](char next) -> bool
            {
                return    (next == ' ')
                       || (next == '\t')
                       || (next == '\n');
            };

        auto is_numeric = [](const std::string& text) -> bool
            {
                if ((text[0] >= '0') && (text[0] <= '9'))
                {
                    return true;
                }

                if (   (text[0] == '-')
                    || (text[0] == '+'))
                {
                    return (text[1] >= '0') && (text[1] <= '9');
                }

                return false;
            };

        auto skip_whitespace = [&]()
            {
                char next = source.peek_next();

                while (source && (is_whitespace(next)))
                {
                    source.next();
                    next = source.peek_next();
                }
            };

        while (source)
        {
            skip_whitespace();

            if (!source)
            {
                break;
            }

            std::string text;
            Token::Type type = Token::Type::word;
            Location location = source.current_location();

            if (source.peek_next() == '"')
            {
                type = Token::Type::string;

                source.next();
                text = get_while([&](char next) -> bool { return next != '"'; });
                source.next();
            }
            else
            {
                text = get_while([&](char next) -> bool { return is_whitespace(next) == false; });
            }

            if (is_numeric(text))
            {
                type = Token::Type::number;
            }

            tokens.push_back({.type = type, .location = location, .text = text });
        }

        return tokens;
    }

    struct DataObject;
    using DataObjectPtr = std::shared_ptr<DataObject>;

    using Value = std::variant<int64_t, double, bool, std::string, DataObjectPtr>;
    using ValueStack = std::list<Value>;
    using ValueList = std::vector<Value>;


    struct DataObjectDefinition
    {
        using NameList = std::vector<std::string>;

        std::string name;
        NameList fieldNames;
    };

    using DataObjectDefinitionPtr = std::shared_ptr<DataObjectDefinition>;
    using DefinitionList = std::vector<DataObjectDefinitionPtr>;

    struct DataObject
    {
        DataObjectDefinitionPtr definition;
        ValueList fields;
    };

    std::ostream& operator <<(std::ostream& stream, const Value& value);

    std::ostream& operator <<(std::ostream& stream, const DataObjectPtr& data)
    {
        if (data)
        {
            stream << "# " << data->definition->name << " ";

            for (int64_t i = 0; i < data->fields.size(); ++i)
            {
                stream << data->definition->fieldNames[i] << ": "
                       << data->fields[i] << " ";
            }

            stream << ";";
        }
        else
        {
            stream << "NULL";
        }

        return stream;
    }

    template <typename variant>
    inline void word_print_if(std::ostream& stream, const Value& next)
    {
        if (const variant* value = std::get_if<variant>(&next))
        {
            stream << *value;
        }
    }


    template <>
    inline void word_print_if<bool>(std::ostream& stream, const Value& next)
    {
        if (const bool* value = std::get_if<bool>(&next))
        {
            stream << std::boolalpha << *value;
        }
    }


    std::ostream& operator <<(std::ostream& stream, const Value& value)
    {
        word_print_if<int64_t>(stream, value);
        word_print_if<double>(stream, value);
        word_print_if<bool>(stream, value);
        word_print_if<std::string>(stream, value);
        word_print_if<DataObjectPtr>(stream, value);

        return stream;
    }


    using SubFunction = std::function<void()>;


    struct Word
    {
        bool immediate = false;
        size_t handler_index;
    };

    using Dictionary = std::unordered_map<std::string, Word>;
    using WordList = std::vector<SubFunction>;

    ValueStack stack;
    ValueList variables;

    DefinitionList data_definitions;

    Dictionary dictionary;
    WordList handlers;

    bool repl_quit = false;

    TokenList input_tokens;
    size_t current_token = 0;

    bool new_word_is_immediate = false;

    bool is_showing_bytecode = false;
    bool is_showing_exec_code = false;

    std::stack<std::filesystem::path> current_path;


    struct OpCode
    {
        enum class Id
        {
            nop,
            push_constant_value,
            exec,
            jump,
            jump_if_zero,
            jump_if_not_zero
        };

        Id id;
        Value value;
    };

    using ByteCode = std::vector<OpCode>;

    std::ostream& operator <<(std::ostream& stream, const OpCode::Id id)
    {
        switch (id)
        {
            case OpCode::Id::nop:                 stream << "nop                "; break;
            case OpCode::Id::push_constant_value: stream << "push_constant_value"; break;
            case OpCode::Id::exec:                stream << "exec               "; break;
            case OpCode::Id::jump:                stream << "jump               "; break;
            case OpCode::Id::jump_if_zero:        stream << "jump_if_zero       "; break;
            case OpCode::Id::jump_if_not_zero:    stream << "jump_if_not_zero   "; break;
        }

        return stream;
    }

    std::ostream& operator <<(std::ostream& stream, const OpCode& op)
    {
        stream << op.id << " " << op.value;
        return stream;
    }

    std::ostream& operator <<(std::ostream& stream, const ByteCode& code)
    {
        for (size_t i = 0; i < code.size(); ++i)
        {
            const auto& op = code[i];
            stream << std::setw(4) << i << " " << op << std::endl;
        }

        return stream;
    }

    struct Construction
    {
        std::string name;
        ByteCode code;
    };

    using ConstructionStack = std::stack<Construction>;

    ConstructionStack construction_stack;


    void set_wd(const std::filesystem::path& path)
    {
        std::filesystem::current_path(path);
        current_path.push(path);
    }


    void reset_wd()
    {
        current_path.pop();
        std::filesystem::current_path(current_path.top());
    }


    [[noreturn]]
    void throw_error(const Location& location, const std::string& message)
    {
        std::stringstream stream;

        stream << "Error: " << location << " - " << message;
        throw std::runtime_error(stream.str());
    }


    Value pop()
    {
        if (stack.empty())
        {
            throw std::runtime_error("Stack underflow.");
        }

        Value next = stack.front();
        stack.pop_front();

        return next;
    }


    void push(const Value& new_value)
    {
        stack.push_front(new_value);
    }


    void add_word(const std::string& word_text, SubFunction handler, bool immediate = false)
    {
        Word new_word = {
                .immediate = immediate,
                .handler_index = handlers.size()
            };

        handlers.push_back(handler);

        auto iter = dictionary.find(word_text);

        if (iter != dictionary.end())
        {
            iter->second = new_word;
        }
        else
        {
            dictionary.insert(Dictionary::value_type(word_text, new_word));
        }
    }


    void process_tokens(const std::string& until);


    void word_quit()
    {
        repl_quit = true;
    }


    void word_word()
    {
        const auto& word = input_tokens[++current_token];
        push(word.text);
    }


    void word_dup()
    {
        Value next = pop();

        push(next);
        push(next);
    }


    void word_drop()
    {
        pop();
    }


    void word_swap()
    {
        auto a = pop();
        auto b = pop();

        push(a);
        push(b);
    }


    void word_over()
    {
        auto a = pop();
        auto b = pop();

        push(a);
        push(b);
        push(a);
    }


    void word_rot()
    {
        auto c = pop();
        auto b = pop();
        auto a = pop();

        push(c);
        push(a);
        push(b);
    }


    template <typename variant>
    variant expect_value_type(Value& value)
    {
        if (std::holds_alternative<variant>(value) == false)
        {
            std::stringstream stream;

            stream << "Unexpected type with value, " << value << ".";
            throw_error({}, stream.str());
        }

        return std::get<variant>(value);
    }


    void word_print()
    {
        Value next = pop();
        std::cout << next << " ";
    }


    void word_newline()
    {
        std::cout << std::endl;
    }


    template <typename variant>
    bool either_is(const Value& a, const Value& b)
    {
        return    std::holds_alternative<variant>(a)
               || std::holds_alternative<variant>(b);
    }


    std::string as_string(const Value& value)
    {
        if (std::holds_alternative<double>(value))
        {
            return std::to_string(std::get<double>(value));
        }

        return std::get<std::string>(value);
    }


    template <typename variant>
    variant as_numeric(const Value& value)
    {
        if (std::holds_alternative<bool>(value))
        {
            return std::get<bool>(value);
        }

        if (std::holds_alternative<int64_t>(value))
        {
            return std::get<int64_t>(value);
        }

        if (std::holds_alternative<double>(value))
        {
            return std::get<double>(value);
        }

        throw_error({}, "Expected numeric or boolean value.");
    }


    void word_hex()
    {
        Value next = pop();
        auto int_value = as_numeric<int64_t>(next);

        std::cout << std::hex << int_value << std::dec << " ";
    }


    void math_op(std::function<double(double, double)> dop,
                 std::function<int64_t(int64_t, int64_t)> iop)
    {
        Value b = pop();
        Value a = pop();
        Value result;

        if (either_is<double>(a, b))
        {
            result = dop(as_numeric<double>(a), as_numeric<double>(b));
        }
        else
        {
            result = iop(as_numeric<int64_t>(a), as_numeric<int64_t>(b));
        }

        push(result);
    }


    void string_or_double_op(std::function<void(double,double)> dop,
                             std::function<void(int64_t,int64_t)> iop,
                             std::function<void(std::string,std::string)> sop)
    {
        auto b = pop();
        auto a = pop();

        if (either_is<std::string>(a, b))
        {
            auto str_a = as_string(a);
            auto str_b = as_string(b);

            sop(str_a, str_b);
        }
        else
        {
            if (either_is<double>(a, b))
            {
                auto a_num = as_numeric<double>(a);
                auto b_num = as_numeric<double>(b);

                dop(a_num, b_num);
            }
            else
            {
                auto a_num = as_numeric<int64_t>(a);
                auto b_num = as_numeric<int64_t>(b);

                iop(a_num, b_num);
            }
        }
    }


    void logic_bit_op(std::function<int64_t(int64_t, int64_t)> op)
    {
        auto b = pop();
        auto a = pop();
        auto result = op(as_numeric<int64_t>(a), as_numeric<int64_t>(b));

        push(result);
    }


    void word_add()
    {
        string_or_double_op([](auto a, auto b) { push(a + b); },
                            [](auto a, auto b) { push(a + b); },
                            [](auto a, auto b) { push(a + b); });
    }


    void word_subtract()
    {
        math_op([](auto a, auto b) -> auto { return a - b; },
                [](auto a, auto b) -> auto { return a - b; });
    }


    void word_multiply()
    {
        math_op([](auto a, auto b) -> auto { return a * b; },
                [](auto a, auto b) -> auto { return a * b; });
    }


    void word_divide()
    {
        math_op([](auto a, auto b) -> auto { return a / b; },
                [](auto a, auto b) -> auto { return a / b; });
    }


    void word_and()
    {
        logic_bit_op([](auto a, auto b) { return a & b; });
    }


    void word_or()
    {
        logic_bit_op([](auto a, auto b) { return a | b; });
    }


    void word_xor()
    {
        logic_bit_op([](auto a, auto b) { return a ^ b; });
    }


    void word_not()
    {
        auto top = pop();
        auto value = as_numeric<int64_t>(top);

        value = ~value;

        push(value);
    }


    void word_equal()
    {
        string_or_double_op([](auto a, auto b) { push((bool)(a == b)); },
                            [](auto a, auto b) { push((bool)(a == b)); },
                            [](auto a, auto b) { push((bool)(a == b)); });
    }


    void word_not_equal()
    {
        string_or_double_op([](auto a, auto b) { push((bool)(a != b)); },
                            [](auto a, auto b) { push((bool)(a != b)); },
                            [](auto a, auto b) { push((bool)(a != b)); });
    }


    void word_greater_equal()
    {
        string_or_double_op([](auto a, auto b) { push((bool)(a >= b)); },
                            [](auto a, auto b) { push((bool)(a >= b)); },
                            [](auto a, auto b) { push((bool)(a >= b)); });
    }


    void word_less_equal()
    {
        string_or_double_op([](auto a, auto b) { push((bool)(a <= b)); },
                            [](auto a, auto b) { push((bool)(a <= b)); },
                            [](auto a, auto b) { push((bool)(a <= b)); });
    }


    void word_greater()
    {
        string_or_double_op([](auto a, auto b) { push((bool)(a > b)); },
                            [](auto a, auto b) { push((bool)(a > b)); },
                            [](auto a, auto b) { push((bool)(a > b)); });
    }


    void word_less()
    {
        string_or_double_op([](auto a, auto b) { push((bool)(a < b)); },
                            [](auto a, auto b) { push((bool)(a < b)); },
                            [](auto a, auto b) { push((bool)(a < b)); });
    }


    void word_variable()
    {
        ++current_token;

        auto name = input_tokens[current_token].text;
        double new_index = variables.size();

        add_word(name, [name, new_index]() { push(new_index); });
        variables.push_back({});
    }


    void word_read_variable()
    {
        Value top = pop();
        auto index = expect_value_type<double>(top);

        push(variables[index]);
    }


    void word_write_variable()
    {
        Value top = pop();
        auto index = expect_value_type<double>(top);

        Value new_value = pop();
        variables[index] = new_value;
    }


    void word_data_definition()
    {
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

        add_word(definition_ptr->name + ".new", [definition_ptr]()
            {
                DataObjectPtr new_object = std::make_shared<DataObject>();

                new_object->definition = definition_ptr;
                new_object->fields.resize(definition_ptr->fieldNames.size());

                push(new_object);
            });

        for (int64_t i = 0; i < definition_ptr->fieldNames.size(); ++i)
        {
            add_word(definition_ptr->name + "." + definition_ptr->fieldNames[i],
                [i]()
                {
                    push(i);
                });
        }
    }


    void word_read_field()
    {
        auto var = pop();
        auto object = expect_value_type<DataObjectPtr>(var);

        var = pop();
        auto field_index = as_numeric<int64_t>(var);

        push(object->fields[field_index]);
    }


    void word_write_field()
    {
        auto var = pop();
        auto object = expect_value_type<DataObjectPtr>(var);

        var = pop();
        auto field_index = as_numeric<int64_t>(var);

        var = pop();
        object->fields[field_index] = var;
    }


    void word_start_function()
    {
        ++current_token;
        auto name = input_tokens[current_token].text;

        construction_stack.push({ .name = name });

        new_word_is_immediate = false;
    }


    void execute_code(const ByteCode& code);


    void word_end_function()
    {
        auto& construction = construction_stack.top();

        add_word(construction.name, [construction]()
            {
                execute_code(construction.code);
            },
            new_word_is_immediate);

        construction_stack.pop();
    }


    void word_immediate()
    {
        new_word_is_immediate = true;
    }


    void compile_token(const Token& token);


    void word_if()
    {
        Construction if_block;
        Construction else_block;

        bool found_else = false;
        bool found_then = false;

        auto if_location = input_tokens[current_token].location;

        construction_stack.push({});
        construction_stack.top().code.push_back({
                .id = OpCode::Id::jump_if_zero,
                .value = 0
            });

        for (++current_token; current_token < input_tokens.size(); ++current_token)
        {
            const Token& token = input_tokens[current_token];

            if (token.text == "else")
            {
                if (found_else)
                {
                    std::stringstream stream;

                    stream << "Duplicate else for if word, " << if_location << ".";
                    throw_error(token.location, stream.str());
                }

                found_else = true;

                if_block = construction_stack.top();
                construction_stack.pop();
                construction_stack.push({});
            }
            else if (token.text == "then")
            {
                found_then = true;

                if (found_else)
                {
                    else_block = construction_stack.top();
                }
                else
                {
                    if_block = construction_stack.top();
                }

                construction_stack.pop();
                break;
            }
            else
            {
                compile_token(token);
            }
        }

        if (!found_then)
        {
            throw_error(if_location, "Missing matching then word for starting if word.");
        }

        if (found_else)
        {
            else_block.code.push_back({
                    .id = OpCode::Id::nop,
                    .value = 0.0
                });

            if_block.code.push_back({
                    .id = OpCode::Id::jump,
                    .value = (int64_t)else_block.code.size()
                });
        }
        else
        {
            if_block.code.push_back({
                    .id = OpCode::Id::nop,
                    .value = 0
                });
        }

        if_block.code[0].value = (int64_t)if_block.code.size();

        auto& base_code = construction_stack.top().code;

        base_code.insert(base_code.end(), if_block.code.begin(), if_block.code.end());
        base_code.insert(base_code.end(), else_block.code.begin(), else_block.code.end());
    }


    void word_loop()
    {
        auto& base_code = construction_stack.top().code;

        Construction loop_block;
        Construction condition_block;

        bool found_until = false;
        bool found_while = false;

        auto begin_location = input_tokens[current_token].location;

        construction_stack.push({});

        for (++current_token; current_token < input_tokens.size(); ++current_token)
        {

            const Token& token = input_tokens[current_token];

            if (token.text == "until")
            {
                if (found_while)
                {
                    std::stringstream stream;

                    stream << "Unexpected until word in while loop, " << begin_location << ".";
                    throw_error(token.location, stream.str());
                }

                loop_block = construction_stack.top();
                construction_stack.pop();

                loop_block.code.push_back({
                        .id = OpCode::Id::jump_if_zero,
                        .value = (int64_t)(0 - loop_block.code.size())
                    });

                base_code.insert(base_code.end(), loop_block.code.begin(), loop_block.code.end());
                break;
            }
            else if (token.text == "while")
            {
                condition_block = construction_stack.top();
                construction_stack.pop();

                found_while = true;
                construction_stack.push({});
            }
            else if (token.text == "repeat")
            {
                if (!found_while)
                {
                    throw_error(begin_location, "Loop missing while word.");
                }

                loop_block = construction_stack.top();
                construction_stack.pop();

                condition_block.code.push_back({
                        .id = OpCode::Id::jump_if_zero,
                        .value = (int64_t)(loop_block.code.size() + 2)
                    });

                loop_block.code.push_back({
                        .id = OpCode::Id::jump,
                        .value = (int64_t)(0 - (condition_block.code.size() +
                                        loop_block.code.size()))
                    });

                loop_block.code.push_back({
                        .id = OpCode::Id::nop,
                        .value = 0.0
                    });

                base_code.insert(base_code.end(),
                                 condition_block.code.begin(),
                                 condition_block.code.end());

                base_code.insert(base_code.end(), loop_block.code.begin(), loop_block.code.end());
                break;
            }
            else
            {
                compile_token(token);
            }
        }
    }


    void word_print_stack()
    {
        for (const auto& value : stack)
        {
            std::cout << value << std::endl;
        }
    }


    void word_print_dictionary()
    {
        size_t max = 0;

        for (const auto& word : dictionary)
        {
            if (max < word.first.size())
            {
                max = word.first.size();
            }
        }

        std::cout << dictionary.size() << " words defined." << std::endl;

        for (const auto& word : dictionary)
        {
            std::cout << std::setw(max) << word.first << " "
                      << std::setw(6) << word.second.handler_index;

            if (word.second.immediate)
            {
                std::cout << " immediate";
            }

            std::cout << std::endl;
        }
    }


    void word_show_bytecode()
    {
        auto value = pop();
        is_showing_bytecode = as_numeric<bool>(value);
    }


    void word_show_exec_code()
    {

        auto value = pop();
        is_showing_exec_code = as_numeric<bool>(value);
    }


    void process_source(const std::filesystem::path& path);


    void word_include()
    {
        auto top = pop();
        std::filesystem::path file_path = expect_value_type<std::string>(top);

        if (!std::filesystem::exists(file_path))
        {
            throw_error({}, "The file " + file_path.string() + " can not be found.");
        }

        process_source(file_path);
    }


    void compile_token(const Token& token)
    {
        auto iter = dictionary.find(token.text);

        if (iter != dictionary.end())
        {
            if (iter->second.immediate)
            {
                auto index = iter->second.handler_index;
                handlers[index]();
            }
            else
            {
                construction_stack.top().code.push_back({
                        .id = OpCode::Id::exec,
                        .value = (double)iter->second.handler_index
                    });
            }
        }
        else
        {
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
                            size_t pos = 2;
                            value = std::stoll(token.text, nullptr, 16);
                        }
                        else
                        {
                            value = std::stoll(token.text);
                        }

                        construction_stack.top().code.push_back({
                                .id = OpCode::Id::push_constant_value,
                                .value = value
                            });
                    }
                    break;

                case Token::Type::string:
                    construction_stack.top().code.push_back({
                            .id = OpCode::Id::push_constant_value,
                            .value = token.text
                        });
                    break;

                case Token::Type::word:
                    throw_error(token.location, "Word '" + token.text + "' not found.");
                    break;
            }
        }
    }


    void compile_token_list(TokenList& input_tokens)
    {
        for (current_token = 0; current_token < input_tokens.size(); ++current_token)
        {
            const Token& token = input_tokens[current_token];
            compile_token(token);
        }
    }


    void execute_code(const ByteCode& code)
    {
        if (is_showing_exec_code)
        {
            std::cout << "-------------------------------------" << std::endl;
        }

        for (size_t pc = 0; pc < code.size(); ++pc)
        {
            const OpCode& op = code[pc];

            if (is_showing_exec_code)
            {
                std::cout << std::setw(4) << pc << " - " << op << std::endl;
            }

            switch (op.id)
            {
                case OpCode::Id::nop:
                    break;

                case OpCode::Id::push_constant_value:
                    push(op.value);
                    break;

                case OpCode::Id::exec:
                    handlers[as_numeric<int64_t>(op.value)]();
                    break;

                case OpCode::Id::jump:
                    pc += as_numeric<int64_t>(op.value) - 1;
                    break;

                case OpCode::Id::jump_if_zero:
                    {
                        auto top = pop();
                        auto value = as_numeric<bool>(top);

                        if (!value)
                        {
                            pc += as_numeric<int64_t>(op.value) - 1;
                        }
                    }
                    break;

                case OpCode::Id::jump_if_not_zero:
                    {
                        auto top = pop();
                        auto value = as_numeric<bool>(top);

                        if (value)
                        {
                            pc += as_numeric<int64_t>(op.value) - 1;
                        }
                    }
                    break;
            }
        }

        if (is_showing_exec_code)
        {
            std::cout << "=====================================" << std::endl;
        }
    }


    void process_source(const std::string& source)
    {
        construction_stack.push({});

        input_tokens = tokenize(source);
        compile_token_list(input_tokens);

        // If the construction stack isn't 1 deep we have a problem.

        if (is_showing_bytecode)
        {
            std::cout << construction_stack.top().code;
        }

        execute_code(construction_stack.top().code);

        construction_stack.pop();
    }


    void process_source(const std::filesystem::path& path)
    {
        auto source_path = std::filesystem::canonical(path);

        auto base_path = source_path;
        base_path.remove_filename();

        set_wd(base_path);

        std::ifstream source_file(source_path);

        auto begin = std::istreambuf_iterator<char>(source_file);
        auto end = std::istreambuf_iterator<char>();

        auto new_source = std::string(begin, end);

        process_source(new_source);

        reset_wd();
    }


    void process_repl()
    {
        std::cout << "Strange Forth REPL." << std::endl;

        while (   (repl_quit == false)
                && (std::cin))
        {
            try
            {
                std::cout << std::endl << "> ";

                std::string line;
                std::getline(std::cin, line);

                process_source(line);

                std::cout << "ok" << std::endl;
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << std::endl;
            }
        }
    }


}



int main(int argc, char* argv[])
{
    try
    {
        add_word("quit", word_quit);

        add_word("word", word_word);

        add_word("dup", word_dup);
        add_word("drop", word_drop);
        add_word("swap", word_swap);
        add_word("over", word_over);
        add_word("rot", word_rot);

        add_word(".", word_print);
        add_word("cr", word_newline);
        add_word(".hex", word_hex);

        add_word("+", word_add);
        add_word("-", word_subtract);
        add_word("*", word_multiply);
        add_word("/", word_divide);

        add_word("and", word_and);
        add_word("or", word_or);
        add_word("xor", word_xor);
        add_word("not", word_not);

        add_word("=", word_equal);
        add_word("<>", word_not_equal);
        add_word(">=", word_greater_equal);
        add_word("<=", word_less_equal);
        add_word(">", word_greater);
        add_word("<", word_less);

        add_word("variable", word_variable, true);
        add_word("@", word_read_variable);
        add_word("!", word_write_variable);

        add_word("#", word_data_definition, true);
        add_word("#@", word_read_field);
        add_word("#!", word_write_field);

        add_word(":", word_start_function, true);
        add_word(";", word_end_function, true);
        add_word("immediate", word_immediate, true);

        add_word("if", word_if, true);
        add_word("begin", word_loop, true);

        add_word(".s", word_print_stack);
        add_word(".w", word_print_dictionary);

        add_word("show_bytecode", word_show_bytecode);
        add_word("show_exec_code", word_show_exec_code);

        add_word("true",  []() { push(true);  });
        add_word("false", []() { push(false); });

        add_word("include", word_include);

        // file_open
        // file_close
        // file!
        // file@
        //

        current_path.push(std::filesystem::current_path());

        auto base_path = std::filesystem::canonical(argv[0]).remove_filename();
        process_source(base_path / "std.f");

        if (argc == 2)
        {
            std::filesystem::path source_path = argv[1];

            if (!std::filesystem::exists(source_path))
            {
                throw std::runtime_error(std::string("File ") + source_path.string() +
                                        " does not exist.");
            }

            process_source(std::filesystem::canonical(source_path));
        }
        else
        {
            process_repl();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
