
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


    using Value = std::variant<double, std::string>;
    using ValueStack = std::stack<Value>;
    using ValueList = std::vector<Value>;


    template <typename variant>
    inline void word_print_if(std::ostream& stream, const Value& next)
    {
        if (const variant* value = std::get_if<variant>(&next))
        {
            std::cout << *value;
        }
    }


    std::ostream& operator <<(std::ostream& stream, const Value& value)
    {
        word_print_if<double>(stream, value);
        word_print_if<std::string>(stream, value);

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

    Dictionary dictionary;
    WordList handlers;

    bool repl_quit = false;

    TokenList input_tokens;
    size_t current_token = 0;


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

        Value next = stack.top();
        stack.pop();

        return next;
    }


    void push(const Value& new_value)
    {
        stack.push(new_value);
    }


    void add_word(const std::string& word_text, SubFunction handler, bool immediate = false)
    {
        dictionary.insert(Dictionary::value_type(word_text,
            {
                .immediate = immediate,
                .handler_index = handlers.size()
            }));

        handlers.push_back(handler);
    }


    void process_tokens(const std::string& until);


    void word_quit()
    {
        repl_quit = true;
    }


    void word_comment()
    {
        auto location = input_tokens[current_token].location;
        bool found_end = false;

        for ( ; current_token < input_tokens.size(); ++current_token)
        {
            if (   (input_tokens[current_token].type == Token::Type::word)
                && (input_tokens[current_token].text == ")"))
            {
                found_end = true;
                break;
            }
        }

        if (!found_end)
        {
            throw_error(location, "Missing end of comment block, ')'.");
        }
    }


    void word_dup()
    {
        Value next = stack.top();
        stack.push(next);
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


    void word_add()
    {
        Value b = pop();
        Value a = pop();

        stack.push(expect_value_type<double>(a) + expect_value_type<double>(b));
    }


    void word_subtract()
    {
        Value b = pop();
        Value a = pop();

        stack.push(expect_value_type<double>(a) - expect_value_type<double>(b));
    }


    void word_multiply()
    {
        Value b = pop();
        Value a = pop();

        stack.push(expect_value_type<double>(a) * expect_value_type<double>(b));
    }


    void word_divide()
    {
        Value b = pop();
        Value a = pop();

        stack.push(expect_value_type<double>(a) / expect_value_type<double>(b));
    }


    void word_variable()
    {
        ++current_token;

        auto name = input_tokens[current_token].text;
        double new_index = variables.size();

        add_word(name, [name, new_index]() { stack.push(new_index); });
        variables.push_back({});
    }


    void word_read_variable()
    {
        Value top = pop();
        auto index = expect_value_type<double>(top);

        stack.push(variables[index]);
    }


    void word_write_variable()
    {
        Value top = pop();
        auto index = expect_value_type<double>(top);

        Value new_value = pop();
        variables[index] = new_value;
    }


    void word_start_function()
    {
        ++current_token;
        auto name = input_tokens[current_token].text;

        construction_stack.push({ .name = name });
    }


    void execute_code(const ByteCode& code);


    void word_end_function()
    {
        auto& construction = construction_stack.top();

        add_word(construction.name, [construction]()
            {
                execute_code(construction.code);
            });

        construction_stack.pop();
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
            .value = 0.0
        });

        for (++current_token; current_token < input_tokens.size(); ++current_token)
        {
            const Token& token = input_tokens[current_token];

            if (token.text == "else")
            {
                if (found_else)
                {
                    throw_error(token.location, "Duplicate else for if word.");
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

        if_block.code[0].value = (double)if_block.code.size();

        auto& base_code = construction_stack.top().code;

        if (found_else)
        {
            else_block.code.push_back({
                .id = OpCode::Id::nop,
                .value = 0.0
            });

            if_block.code[0].value = std::get<double>(if_block.code[0].value) + 1.0;
            if_block.code.push_back({
                .id = OpCode::Id::jump,
                .value = (double)(if_block.code.size() + else_block.code.size())
            });
        }
        else
        {
            if_block.code.push_back({
                .id = OpCode::Id::nop,
                .value = 0.0
            });
        }

        base_code.insert(base_code.end(), if_block.code.begin(), if_block.code.end());
        base_code.insert(base_code.end(), else_block.code.begin(), else_block.code.end());
    }


    void compile_token(const Token& token)
    {
        switch (token.type)
        {
            case Token::Type::number:
                construction_stack.top().code.push_back({
                    .id = OpCode::Id::push_constant_value,
                    .value = std::stod(token.text)
                });
                break;

            case Token::Type::string:
                construction_stack.top().code.push_back({
                    .id = OpCode::Id::push_constant_value,
                    .value = token.text
                });
                break;

            case Token::Type::word:
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
                        throw_error(token.location, "Word '" + token.text + "' not found.");
                    }
                }
                break;
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
        for (size_t pc = 0; pc < code.size(); ++pc)
        {
            const OpCode& op = code[pc];

            switch (op.id)
            {
                case OpCode::Id::nop:
                    break;

                case OpCode::Id::push_constant_value:
                    push(op.value);
                    break;

                case OpCode::Id::exec:
                    handlers[(size_t)std::get<double>(op.value)]();
                    break;

                case OpCode::Id::jump:
                    pc = (size_t)std::get<double>(op.value) - 1;
                    break;

                case OpCode::Id::jump_if_zero:
                    {
                        auto value = pop();

                        if (expect_value_type<double>(value) == 0.0)
                        {
                            pc = (size_t)std::get<double>(op.value) - 1;
                        }
                    }
                    break;

                case OpCode::Id::jump_if_not_zero:
                    {
                        auto value = pop();

                        if (expect_value_type<double>(value) != 0.0)
                        {
                            pc = (size_t)std::get<double>(op.value) - 1;
                        }
                    }
                    break;
            }
        }
    }


    void process_source(const std::string& source)
    {
        construction_stack.push({});

        input_tokens = tokenize(source);
        compile_token_list(input_tokens);

        // If the construction stack isn't 1 deep we have a problem.

        execute_code(construction_stack.top().code);

        construction_stack.pop();
    }


    void process_source(const std::filesystem::path& path)
    {
        std::ifstream source_file(path);

        auto begin = std::istreambuf_iterator<char>(source_file);
        auto end = std::istreambuf_iterator<char>();

        auto new_source = std::string(begin, end);

        process_source(new_source);
    }


    void process_repl()
    {
        std::cout << "Silly Forth REPL." << std::endl;

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

        add_word("(", word_comment, true);

        add_word("dup", word_dup);

        add_word(".", word_print);
        add_word("cr", word_newline);

        add_word("+", word_add);
        add_word("-", word_subtract);
        add_word("*", word_multiply);
        add_word("/", word_divide);

        add_word("variable", word_variable, true);
        add_word("@", word_read_variable);
        add_word("!", word_write_variable);

        add_word(":", word_start_function, true);
        add_word(";", word_end_function, true);

        add_word("if", word_if, true);

        auto base_path = std::filesystem::canonical(argv[0]).remove_filename() / "std.sorth";

        process_source(base_path);
        process_repl();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
