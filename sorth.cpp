
// Implementation of an interpreted version of Forth.  This version is aware of the standard ANS
// Forth however we don't adhere to it very closely.  We've taken liberties to follow some idioms
// from newer languages where the author finds it enhances readability.  It's acknowledged that this
// is an entirely subjective standard.
//
// The second reason why this version deviates from most Forth implementations is that this version
// doesn't compile to machine code.  Instead this version compiles and runs as a bytecode
// interpreter, and also makes use of exceptions and features available in newer versions of C++ as
// opposed to mostly being implemented in assembly.
//
// The knock on effects of these decisions is that this version of Form may be slower than other
// compiled versions.  The intention of this is for this version to act more as a scripting language
// rather than as a compiler/executable generator with optional REPL.

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
#include <cassert>



namespace
{


    // First off, we need to keep track of where we are in the source code for error reporting.  We
    // use a shared_ptr for the path component in order to avoid duplicating a path string all over
    // the place.

    using PathPtr = std::shared_ptr<std::filesystem::path>;


    // Simple wrappers create this shared path.

    inline PathPtr shared_path(const char* string)
    {
        return std::make_shared<std::filesystem::path>(string);
    }


    inline PathPtr shared_path(const std::string& string)
    {
        return std::make_shared<std::filesystem::path>(string);
    }


    inline PathPtr shared_path(const std::filesystem::path& path)
    {
        return std::make_shared<std::filesystem::path>(path);
    }


    class Location
    {
        private:
            PathPtr source_path;
            size_t line;
            size_t column;

        public:
            Location()
            : line(1),
              column(1)
            {
            }

            Location(const PathPtr& path)
            : source_path(path),
              line(1),
              column(1)
            {
            }

        public:
            const PathPtr& get_path() const noexcept
            {
                return source_path;
            }

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


    // We format errors more like gcc so that code editors have an easier time of jumping to the
    // location the error actually occurred.

    std::ostream& operator <<(std::ostream& stream, Location location)
    {
        auto& path = location.get_path();

        if (path)
        {
            stream << *path << ":";
        }

        stream << location.get_line() << ":" << location.get_column();

        return stream;
    }


    [[noreturn]]
    inline void throw_error(const Location& location, const std::string& message)
    {
        std::stringstream stream;

        stream << location << ": Error: " << message;
        throw std::runtime_error(stream.str());
    }


    inline void throw_error_if(bool condition, const Location& location, const std::string& message)
    {
        if (condition)
        {
            throw_error(location, message);
        }
    }


    // Helper class that keeps track of the current location as we tokenize the source code.  It'll
    // also take care of loading the source from a file on disk.  Or just simply accept text from
    // the REPL.

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
              source_location(shared_path("<repl>"))
            {
            }

            SourceBuffer(std::string const& new_source)
            : source(new_source),
              position(0),
              source_location(shared_path("<repl>"))
            {
            }

            SourceBuffer(const std::filesystem::path& path)
            : source(),
              position(0),
              source_location(shared_path(path))
            {
                std::ifstream source_file(*source_location.get_path());

                auto begin = std::istreambuf_iterator<char>(source_file);
                auto end = std::istreambuf_iterator<char>();

                source = std::move(std::string(begin, end));

                // TODO: Look for a #! on the first line and remove it before the rest of the
                //       interpreter sees it.
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


    // Take the source in a source buffer and convert it into a list of tokens, or words really.
    // This is used by the native compiler and also the interpreted code to manipulate the language
    // itself.  One of the fun things about Forth is that it can help define itself.

    struct Token
    {
        // The tokenizer's best guess of the meaning of the given token.  This is really optional as
        // it may be reinterpreted later.
        enum class Type
        {
            number,
            string,
            word
        };

        Type type;          // What kind of token do we think this is?
        Location location;  // Where in the source was this token found?
        std::string text;   // The actual token itself.
    };


    std::ostream& operator <<(std::ostream& stream, Token token) noexcept
    {
        stream << token.location << ": " << token.text;

        return stream;
    }


    using TokenList = std::vector<Token>;


    TokenList tokenize(SourceBuffer& source_code)
    {
        TokenList tokens;

        auto get_while = [&](std::function<bool(char)> test) -> std::string
            {
                std::string new_string;
                char next = source_code.peek_next();

                while (    (source_code)
                        && (test(next)))
                {
                    new_string += source_code.next();
                    next = source_code.peek_next();
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
                char next = source_code.peek_next();

                while (source_code && (is_whitespace(next)))
                {
                    source_code.next();
                    next = source_code.peek_next();
                }
            };

        while (source_code)
        {
            skip_whitespace();

            if (!source_code)
            {
                break;
            }

            std::string text;
            Token::Type type = Token::Type::word;
            Location location = source_code.current_location();

            if (source_code.peek_next() == '"')
            {
                type = Token::Type::string;

                source_code.next();
                text = get_while([&](char next) -> bool { return next != '"'; });
                source_code.next();
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


    // All of the types that ther interpreter can natively understand are represented in the value
    // variant.  With int64_t being defined first, all values will default to the integer value of
    // zero.
    using Value = std::variant<int64_t, double, bool, std::string, Token, Location>;
    using ValueStack = std::stack<Value>;


    // Let's make sure we can convert values to text for displaying to the user among various other
    // uses like writing to a text file.
    template <typename variant>
    inline void value_print_if(std::ostream& stream, const Value& variant_value) noexcept
    {
        if (const variant* value = std::get_if<variant>(&variant_value))
        {
            stream << *value;
        }
    }


    template <>
    inline void value_print_if<bool>(std::ostream& stream, const Value& variant_value) noexcept
    {
        if (const bool* value = std::get_if<bool>(&variant_value))
        {
            stream << std::boolalpha << *value;
        }
    }


    std::ostream& operator <<(std::ostream& stream, const Value& value) noexcept
    {
        value_print_if<int64_t>(stream, value);
        value_print_if<double>(stream, value);
        value_print_if<bool>(stream, value);
        value_print_if<std::string>(stream, value);
        value_print_if<Token>(stream, value);
        //value_print_if<DataObjectPtr>(stream, value);

        return stream;
    }


    // The contextual list allows the interpreter to keep track of various contexts or scopes.
    // Variables and other forms of data are kept track of by indexable lists.  The idea being that
    // the list can expand as new items are added.  We extend this concept with contexts or scopes
    // that when a given context is exited, all values in that context are forgotten.

    extern Location current_location;

    template <typename value_type>
    class ContextualList
    {
        private:
            struct SubList
            {
                std::vector<value_type> items;
                size_t start_index;
            };

            using ListStack = std::list<SubList>;

            ListStack stack;

        public:
            ContextualList()
            {
                // Make sure we have an empty list ready to be populated.
                mark_context();
            }

        public:
            size_t size() const noexcept
            {
                return stack.front().start_index + stack.front().items.size();
            }

            size_t insert(const value_type& value)
            {
                stack.front().items.push_back(value);
                return size() - 1;
            }

            value_type& operator [](size_t index)
            {
                if (index >= size())
                {
                    throw_error(current_location, "Index out of range.");
                }

                for (auto stack_iter = stack.begin(); stack_iter != stack.end(); ++stack_iter)
                {
                    if (index >= stack_iter->start_index)
                    {
                        index -= stack_iter->start_index;
                        return stack_iter->items[index];
                    }
                }

                throw_error(current_location, "Index not found.");
            }

        public:
            void mark_context() noexcept
            {
                size_t start_index = 0;

                if (!stack.empty())
                {
                    start_index = stack.front().start_index + stack.front().items.size();
                }

                stack.push_front({ .items = {}, .start_index = start_index });
            }

            void release_context() noexcept
            {
                stack.pop_front();
            }
    };


    // The Forth dictionary.  Handlers for Forth words are not stored directly in the dictionary.
    // Instead they are stored in their own list and the index and any important flags are what is
    // stored in the dictionary directly.
    //
    // Also note that the dictionary is implemented as a stack of dictionaries.  This allows the
    // Forth to contain scopes.  If a word is redefined in a higher scope, it effectively replaces
    // that word until that scope is released.

    struct Word
    {
        bool immediate = false;  // Should this word be executed at compile or at run time?
                                 // True indicates that the word is executed at compile time.
        size_t handler_index;    // Index of the handler to execute.
    };


    class Dictionary
    {
        private:
            using SubDictionary = std::unordered_map<std::string, Word>;
            using DictionaryStack = std::list<SubDictionary>;

            DictionaryStack stack;

        public:
            Dictionary()
            {
                // Start with an empty dictionary.
                mark_context();
            }

        public:
            void insert(const std::string& text, const Word& value) noexcept
            {
                auto& current_dictionary = stack.front();
                auto iter = current_dictionary.find(text);

                if (iter != current_dictionary.end())
                {
                    iter->second = value;
                }
                else
                {
                    current_dictionary.insert(SubDictionary::value_type(text, value));
                }
            }

            std::tuple<bool, Word> find(const std::string& word) const
            {
                for (auto stack_iter = stack.begin(); stack_iter != stack.end(); ++stack_iter)
                {
                    auto iter = stack_iter->find(word);

                    if (iter != stack_iter->end())
                    {
                        return { true, iter->second };
                    }
                }

                return { false, {} };
            }

        public:
            void mark_context() noexcept
            {
                // Create a new dictionary.
                stack.push_front({});
            }

            void release_context()
            {
                stack.pop_front();

                // There should always be at least one dictionary.  If there isn't something has
                // gone horribly wrong.
                assert(!stack.empty());
            }

        protected:
            friend std::ostream& operator <<(std::ostream& stream, const Dictionary& dictionary);
    };


    std::ostream& operator <<(std::ostream& stream, const Dictionary& dictionary)
    {
        return stream;
    }


    using WordFunction = std::function<void()>;
    using WordList = ContextualList<WordFunction>;


    // All of the interpreter's internal state is kept here.  This state is exposed as globals in
    // order to make it easier to define words both native and interpreted that can manipulate this
    // internal state.  It's one of the beauties of Forth that the language is extendable by the
    // language itself.  By exposing this state as Forth words, we make this extension easier.

    bool is_interpreter_quitting = false;
    int exit_code = EXIT_SUCCESS;

    bool is_showing_bytecode = false;  // Debugging: Show bytecode on stderr as it's generated.
    bool is_showing_run_code = false;  // Debugging: Show bytecode on stderr as it's about to be
                                       //            executed.
    ValueStack stack;  // The interpreter's runtime data stack.  The return stack is taken care of
                       // by the native C++ stack.

    Location current_location;  // The last source code location noted by the bytecode interpreter.

    Dictionary dictionary;   // All of the words known to the language.
    WordList word_handlers;  // The functions that are executed for those words.


    void mark_context()
    {
        dictionary.mark_context();
        word_handlers.mark_context();
    }


    void release_context()
    {
        dictionary.release_context();
        word_handlers.release_context();
    }


    // Stack manipulation.  It's handy to concentrate the language semantics here.

    Value pop()
    {
        if (stack.empty())
        {
            throw_error(current_location, "Stack underflow.");
        }

        auto next = stack.top();
        stack.pop();

        return next;
    }


    inline void push(const Value& value)
    {
        stack.push(value);
    }


    // Value interrogation and conversion functions.  Because the value itself is a variant, it's
    // helpful to have functions that will handle extraction into native types according to the
    // semantics of the language being implemented.

    template <typename variant>
    bool either_is(const Value& a, const Value& b)
    {
        return    std::holds_alternative<variant>(a)
               || std::holds_alternative<variant>(b);
    }


    bool is_numeric(const Value& value)
    {
        return    std::holds_alternative<bool>(value)
               || std::holds_alternative<int64_t>(value)
               || std::holds_alternative<double>(value);
    }


    bool is_string(const Value& value)
    {
        return    std::holds_alternative<std::string>(value)
               || std::holds_alternative<Token>(value);
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

        throw_error(current_location, "Expected numeric or boolean value.");
    }


    std::string as_string(const Value& value)
    {
        if (std::holds_alternative<int64_t>(value))
        {
            return std::to_string(std::get<int64_t>(value));
        }

        if (std::holds_alternative<double>(value))
        {
            return std::to_string(std::get<double>(value));
        }

        if (std::holds_alternative<bool>(value))
        {
            if (std::get<bool>(value))
            {
                return "true";
            }

            return "false";
        }

        if (std::holds_alternative<Token>(value))
        {
            return std::get<Token>(value).text;
        }

        if (std::holds_alternative<std::string>(value))
        {
            return std::get<std::string>(value);
        }

        throw_error(current_location, "No string conversion for value.");
    }


    // Pieces of the bytecode interpreter itself.  Here we define the raw operations of the
    // interpreter and the functions that execute those operations.

    struct OperationCode
    {
        enum class Id : unsigned char
        {
            nop,
            execute,
            push_constant_value,
            location
        };

        Id id;
        Value value;
    };


    using ByteCode = std::vector<OperationCode>;  // Operations to be executed by the interpreter.


    std::ostream& operator <<(std::ostream& stream, const OperationCode::Id id) noexcept
    {
        switch (id)
        {
            case OperationCode::Id::nop:                 stream << "nop                "; break;
            case OperationCode::Id::execute:             stream << "execute            "; break;
            case OperationCode::Id::push_constant_value: stream << "push_constant_value"; break;
            case OperationCode::Id::location:            stream << "location           "; break;
        }

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const OperationCode& op) noexcept
    {
        stream << op.id << " " << op.value;

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const ByteCode& code)
    {
        for (size_t i = 0; i < code.size(); ++i)
        {
            const auto& op = code[i];
            stream << std::setw(6) << i << " " << op << std::endl;
        }

        return stream;
    }


    void execute_code(ByteCode& code)
    {
        if (is_showing_run_code)
        {
            std::cout << "-------------------------------------" << std::endl;
        }

        for (size_t pc = 0; pc < code.size(); ++pc)
        {
            OperationCode& operation = code[pc];

            if (is_showing_run_code)
            {
                std::cout << std::setw(6) << pc << " - " << operation << std::endl;
            }

            switch (operation.id)
            {
                case OperationCode::Id::nop:
                    break;

                case OperationCode::Id::execute:
                    if (is_string(operation.value))
                    {
                        auto [found, word] = dictionary.find(as_string(operation.value));

                        if (!found)
                        {
                            throw_error(current_location, "Word not found.");
                        }

                        word_handlers[word.handler_index]();
                    }
                    else if (is_numeric(operation.value))
                    {
                        auto index = as_numeric<int64_t>(operation.value);
                        word_handlers[index]();
                    }
                    else
                    {
                        throw_error(current_location, "Can no execute unexpected value type.");
                    }
                    break;

                case OperationCode::Id::push_constant_value:
                    push(operation.value);
                    break;

                case OperationCode::Id::location:
                    current_location = std::get<Location>(operation.value);
                    break;
            }
        }

        if (is_showing_run_code)
        {
            std::cout << "=====================================" << std::endl;
        }
    }


    // The bytecode compiler functionality.  Here we take incoming textual words and convert them to
    // bytecode to be executed later.

    struct Construction
    {
        std::string name;
        ByteCode code;
    };

    using ConstructionStack = std::stack<Construction>;


    ConstructionStack construction_stack;
    size_t current_token;

    TokenList input_tokens;

    std::stack<std::filesystem::path> current_path;  // Keep track of previous working directories
                                                     // as we change the current one.


    void compile_token(const Token& token)
    {
        // In Forth anything can be a word, so first we see if it's defined in the dictionary.  If
        // it is, we either compile or execute the word depending on if it's an immediate.
        auto [found, word] = dictionary.find(token.text);

        if (found)
        {
            if (word.immediate)
            {
                word_handlers[word.handler_index]();
            }
            else
            {
                construction_stack.top().code.push_back({
                        .id = OperationCode::Id::execute,
                        .value = (int64_t)word.handler_index
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
                            value = std::stoll(token.text, nullptr, 16);
                        }
                        else
                        {
                            value = std::stoll(token.text);
                        }

                        construction_stack.top().code.push_back({
                                .id = OperationCode::Id::push_constant_value,
                                .value = value
                            });
                    }
                    break;

                case Token::Type::string:
                    construction_stack.top().code.push_back({
                            .id = OperationCode::Id::push_constant_value,
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

        if (is_showing_bytecode)
        {
            std::cerr << construction_stack.top().code;
        }
    }


    // REPL and source file handling.

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


    void process_source(SourceBuffer& buffer)
    {
        construction_stack.push({});

        input_tokens = tokenize(buffer);
        compile_token_list(input_tokens);

        execute_code(construction_stack.top().code);

        construction_stack.pop();
    }


    void process_source(const std::string& source_text)
    {
        SourceBuffer source(source_text);
        process_source(source);
    }


    void process_source(const std::filesystem::path& path)
    {
        auto source_path = std::filesystem::canonical(path);

        auto base_path = source_path;
        base_path.remove_filename();

        SourceBuffer source(path);

        set_wd(base_path);
        process_source(source);
        reset_wd();
    }


    void process_repl()
    {
        std::cout << "Strange Forth REPL." << std::endl;

        while (   (is_interpreter_quitting == false)
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
            catch (const std::exception& e)
            {
                std::cerr << e.what() << std::endl;
            }
        }
    }


    // All of the built in words are defined here.

    void add_word(const std::string& word, std::function<void()> handler,
                  bool immediate = false) noexcept
    {
        dictionary.insert(word, {
                .immediate = immediate,
                .handler_index = word_handlers.insert(handler)
            });
    }


    void word_quit() noexcept
    {
        is_interpreter_quitting = true;

        if (!stack.empty())
        {
            auto value = pop();
            exit_code = as_numeric<int64_t>(value);
        }
    }


    void word_reset() noexcept
    {
        // Clear the current context and remake a new blank one.
        release_context();
        mark_context();
    }


    void word_include()
    {
        auto top = pop();
        std::filesystem::path file_path = as_string(top);

        if (!std::filesystem::exists(file_path))
        {
            throw_error(current_location, "The file " + file_path.string() + " can not be found.");
        }

        process_source(file_path);
    }


    void word_exit_success() noexcept
    {
        push(EXIT_SUCCESS);
    }


    void word_exit_failure() noexcept
    {
        push(EXIT_FAILURE);
    }


    void word_true() noexcept
    {
        push(true);
    }


    void word_false() noexcept
    {
        push(false);
    }


    // Gather up all the native words and make them available to the interpreter.

    void init_builtin_words() noexcept
    {
        // Words for changing interpreter state.
        add_word("quit", word_quit);                  // ( <optional> exit_value -- )
        add_word("reset", word_reset);                // ( -- )
        add_word("include", word_include);            // ( source_path -- <result_from_script> )

        // Some built in constants.
        add_word("exit_success", word_exit_success);  // ( -- exit_success )
        add_word("exit_failure", word_exit_failure);  // ( -- exit_fail )
        add_word("true", word_true);                  // ( -- true )
        add_word("false", word_false);                // ( -- false )
    }


}



int main(int argc, char* argv[])
{
    try
    {
        current_path.push(std::filesystem::current_path());

        init_builtin_words();
        mark_context();

        process_repl();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return exit_code;
}
