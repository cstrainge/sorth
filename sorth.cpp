
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
// The knock on effects of these decisions is that this version of Forth may be slower than other
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
#include <map>
#include <cassert>
#include <optional>



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

            Location(const PathPtr& path, size_t new_line, size_t new_column)
            : source_path(path),
              line(new_line),
              column(new_column)
            {
            }

        public:
            const PathPtr& get_path() const
            {
                return source_path;
            }

            size_t get_line() const
            {
                return line;
            }

            size_t get_column() const
            {
                return column;
            }

        public:
            void next()
            {
                ++column;
            }

            void next_line()
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
            stream << path->string() << ":";
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
            operator bool() const
            {
                return position < source.size();
            }

        public:
            char peek_next() const
            {
                if (position >= source.size())
                {
                    return ' ';
                }

                return source[position];
            }

            char next()
            {
                auto next = peek_next();

                if (*this)
                {
                    increment_position(next);
                }

                return next;
            }

            Location current_location() const
            {
                return source_location;
            }

        private:
            void increment_position(char next)
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


    std::ostream& operator <<(std::ostream& stream, Token token)
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

            if (   (type != Token::Type::string)
                && (is_numeric(text)))
            {
                type = Token::Type::number;
            }

            tokens.push_back({.type = type, .location = location, .text = text });
        }

        return tokens;
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
            size_t size() const
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
            void mark_context()
            {
                size_t start_index = 0;

                if (!stack.empty())
                {
                    start_index = stack.front().start_index + stack.front().items.size();
                }

                stack.push_front({ .items = {}, .start_index = start_index });
            }

            void release_context()
            {
                stack.pop_front();
            }
    };


    using WordFunction = std::function<void()>;

    struct WordHandlerInfo
    {
        std::string name;
        WordFunction function;
        Location definition_location;
    };

    using WordList = ContextualList<WordHandlerInfo>;


    // The Forth dictionary.  Handlers for Forth words are not stored directly in the dictionary.
    // Instead they are stored in their own list and the index and any important flags are what is
    // stored in the dictionary directly.
    //
    // Also note that the dictionary is implemented as a stack of dictionaries.  This allows the
    // Forth to contain scopes.  If a word is redefined in a higher scope, it effectively replaces
    // that word until that scope is released.

    struct Word
    {
        bool is_immediate;     // Should this word be executed at compile or at run time?  True
                               // indicates that the word is executed at compile time.
        bool is_scripted;      // Was this word defined in Forth?
        size_t handler_index;  // Index of the handler to execute.
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
            void insert(const std::string& text, const Word& value)
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
            void mark_context()
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
            friend std::vector<std::string> inverse_lookup_list(const Dictionary& dictionary,
                                                                const WordList& word_handlers);
            friend std::ostream& operator <<(std::ostream& stream, const Dictionary& dictionary);
    };


    std::vector<std::string> inverse_lookup_list(const Dictionary& dictionary,
                                                 const WordList& word_handlers)
    {
        std::vector<std::string> name_list;

        name_list.resize(word_handlers.size());

        for (auto iter = dictionary.stack.begin(); iter != dictionary.stack.end(); ++iter)
        {
            for (auto word_iter = iter->begin(); word_iter != iter->end(); ++word_iter)
            {
                name_list[word_iter->second.handler_index] = word_iter->first;
            }
        }

        return name_list;
    }


    std::ostream& operator <<(std::ostream& stream, const Dictionary& dictionary)
    {
        // First merge all the sub-dictionaries into one sorted dictionary.  Note that words will
        // appear only once.  Even if they are redefined at higher scopes.  Only the newest version
        // will be displayed.
        std::map<std::string, Word> new_dictionary;

        for (auto iter = dictionary.stack.begin(); iter != dictionary.stack.end(); ++iter)
        {
            const Dictionary::SubDictionary& sub_dictionary = *iter;
            new_dictionary.insert(sub_dictionary.begin(), sub_dictionary.end());
        }

        // For formatting, find out the largest word width.

        size_t max = 0;

        for (const auto& word : new_dictionary)
        {
            if (max < word.first.size())
            {
                max = word.first.size();
            }
        }

        // Now, print out the consolidated dictionary.

        std::cout << new_dictionary.size() << " words defined." << std::endl;

        for (const auto& word : new_dictionary)
        {
            std::cout << std::setw(max) << word.first << " "
                      << std::setw(6) << word.second.handler_index;

            if (word.second.is_immediate)
            {
                std::cout << " immediate";
            }

            std::cout << std::endl;
        }

        return stream;
    }


    // All of the types that the interpreter can natively understand are represented in the value
    // variant.  With int64_t being defined first, all values will default to the integer value of
    // zero.

    struct DataObject;
    using DataObjectPtr = std::shared_ptr<DataObject>;

    class Array;
    using ArrayPtr = std::shared_ptr<Array>;

    class ByteBuffer;
    using ByteBufferPtr = std::shared_ptr<ByteBuffer>;


    using Value = std::variant<int64_t, double, bool, std::string, Token, Location, DataObjectPtr,
                               ArrayPtr, ByteBufferPtr>;

    using ValueStack = std::list<Value>;
    using ValueList = std::vector<Value>;

    using VariableList = ContextualList<Value>;


    class Array
    {
        private:
            std::vector<Value> items;

        public:
            Array(int64_t size)
            {
                items.resize(size);
            }

        public:
            inline int64_t size() const
            {
                return items.size();
            }

            inline Value& operator [](int64_t index)
            {
                return items[index];
            }

            inline void grow(const Value& value)
            {
                items.push_back(value);
            }
    };


    class ByteBuffer
    {
        private:
            std::vector<unsigned char> bytes;
            int64_t position;

        public:
            ByteBuffer(int64_t size)
            : position(0)
            {
                bytes.resize(size);
            }

        public:
            int64_t size() const
            {
                return bytes.size();
            }

            int64_t postion() const
            {
                return position;
            }

            void set_position(int64_t new_position)
            {
                position = new_position;
            }

        public:
            void write_int(int64_t byte_size, int64_t value)
            {
                void* data_ptr =(&bytes[position]);
                memcpy(data_ptr, &value, byte_size);

                position += byte_size;
            }

            int64_t read_int(int64_t byte_size, bool is_signed)
            {
                int64_t value = 0;
                void* data_ptr =(&bytes[position]);
                memcpy(&value, data_ptr, byte_size);

                if (is_signed)
                {
                    auto bit_size = byte_size * 8;
                    auto sign_flag = 1 << (bit_size - 1);

                    if ((value & sign_flag) != 0)
                    {
                        uint64_t negative_bits = 0xffffffffffffffff << (64 - bit_size);
                        value = value | negative_bits;
                    }
                }

                position += byte_size;

                return value;
            }

            void write_float(int64_t byte_size, double value)
            {
                void* data_ptr =(&bytes[position]);

                if (byte_size == 4)
                {
                    float float_value = (float)value;

                    memcpy(data_ptr, &float_value, 4);
                }
                else
                {
                    memcpy(data_ptr, &value, 8);
                }

                position += byte_size;
            }

            double read_float(int64_t byte_size)
            {
                double new_value = 0.0;
                void* data_ptr =(&bytes[position]);

                if (byte_size == 4)
                {
                    float float_value = 0.0;

                    memcpy(&float_value, data_ptr, 4);
                    new_value = float_value;
                }
                else
                {
                    memcpy(&new_value, data_ptr, 8);
                }

                position += byte_size;

                return new_value;
            }

        private:
            friend std::ostream& operator <<(std::ostream& stream, const ByteBuffer& buffer);
    };


    std::ostream& operator <<(std::ostream& stream, const ByteBuffer& buffer)
    {
        stream << "          00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f";

        for (size_t i = 0; i < buffer.bytes.size(); ++i)
        {
            if ((i == 0) || ((i % 16) == 0))
            {
                stream << std::endl << std::setw(8) << std::setfill('0') << std::hex
                       << i << "  ";
            }

            stream << std::setw(2) << std::setfill('0') << std::hex
                   << (uint32_t)buffer.bytes[i] << " ";
        }

        stream << std::dec << std::setfill(' ');

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const ByteBufferPtr& buffer_ptr)
    {
        stream << *buffer_ptr;

        return stream;
    }


    // The base definition of a data object, useful for reflection and creation of the actual data
    // objects.

    struct DataObjectDefinition
    {
        using NameList = std::vector<std::string>;

        std::string name;     // The name of the type.
        NameList fieldNames;  // Names of all the fields.
    };

    using DataObjectDefinitionPtr = std::shared_ptr<DataObjectDefinition>;
    using DefinitionList = ContextualList<DataObjectDefinitionPtr>;

    struct DataObject
    {
        DataObjectDefinitionPtr definition;  // Reference of the base definition.
        ValueList fields;                    // The actual values of the structure.
    };


    std::ostream& operator <<(std::ostream& stream, const Value& value) ;


    // When we print out a data structure we include the definition so that we can include field
    // names along with the name of the type itself.
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


    // Let's make sure we can convert values to text for displaying to the user among various other
    // uses like writing to a text file.
    template <typename variant>
    inline void value_print_if(std::ostream& stream, const Value& variant_value)
    {
        if (const variant* value = std::get_if<variant>(&variant_value))
        {
            stream << *value;
        }
    }


    template <>
    inline void value_print_if<bool>(std::ostream& stream, const Value& variant_value)
    {
        if (const bool* value = std::get_if<bool>(&variant_value))
        {
            stream << std::boolalpha << *value;
        }
    }


    std::ostream& operator <<(std::ostream& stream, const Value& value)
    {
        value_print_if<int64_t>(stream, value);
        value_print_if<double>(stream, value);
        value_print_if<bool>(stream, value);
        value_print_if<std::string>(stream, value);
        value_print_if<Token>(stream, value);
        value_print_if<DataObjectPtr>(stream, value);
        value_print_if<Location>(stream, value);

        value_print_if<ByteBufferPtr>(stream, value);

        return stream;
    }


    struct CallItem
    {
        Location word_location;
        std::string word_name;
    };


    using CallStack = std::list<CallItem>;


    std::ostream& operator <<(std::ostream& stream, const CallStack& call_stack)
    {
        stream << std::endl;

        for (const auto& item : call_stack)
        {
            stream << "    " << item.word_location << " -- " << item.word_name << std::endl;
        }

        return stream;
    }


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
    CallStack call_stack;

    Dictionary dictionary;   // All of the words known to the language.
    WordList word_handlers;  // The functions that are executed for those words.

    VariableList variables;  // List of all the allocated variables.

    DefinitionList definitions;  // List of all defined data structures.



    void call_stack_push(const std::string& name, const Location& location)
    {
        call_stack.push_front({
                .word_location = location,
                .word_name = name
            });
    }


    void call_stack_push(const WordHandlerInfo& handler_info)
    {
        call_stack_push(handler_info.name, handler_info.definition_location);
    }


    void call_stack_pop()
    {
        call_stack.pop_front();
    }


    void mark_context()
    {
        dictionary.mark_context();
        word_handlers.mark_context();
        variables.mark_context();
        definitions.mark_context();
    }


    void release_context()
    {
        dictionary.release_context();
        word_handlers.release_context();
        variables.release_context();
        definitions.release_context();
    }


    // Stack manipulation.  It's handy to concentrate the language semantics here.

    Value pop()
    {
        if (stack.empty())
        {
            throw_error(current_location, "Stack underflow.");
        }

        auto next = stack.front();
        stack.pop_front();

        return next;
    }


    inline void push(const Value& value)
    {
        stack.push_front(value);
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


    ByteBufferPtr as_byte_buffer(Value value)
    {
        if (!std::holds_alternative<ByteBufferPtr>(value))
        {
            throw_error(current_location, "Expected a byte buffer.");
        }

        return std::get<ByteBufferPtr>(value);
    }


    void check_buffer_index(const ByteBufferPtr& buffer, int64_t byte_size)
    {
        if ((buffer->postion() + byte_size) > buffer->size())
        {
            std::stringstream stream;

            stream << "Writing a value of size " << byte_size << " at a position of "
                   << buffer->postion() << " would exceed the buffer size, "
                   << buffer->size() << ".";

            throw_error(current_location, stream.str());
        }
    }


    // Pieces of the bytecode interpreter itself.  Here we define the raw operations of the
    // interpreter and the functions that execute those operations.

    struct OperationCode
    {
        enum class Id : unsigned char
        {
            def_variable,
            def_constant,
            read_variable,
            write_variable,
            execute,
            word_index,
            push_constant_value,
            jump,
            jump_if_zero,
            jump_if_not_zero,
            jump_target
        };

        Id id;
        Value value;
        std::optional<Location> location;
    };


    using ByteCode = std::vector<OperationCode>;  // Operations to be executed by the interpreter.


    std::ostream& operator <<(std::ostream& stream, const OperationCode::Id id)
    {
        switch (id)
        {
            case OperationCode::Id::def_variable:        stream << "def_variable       "; break;
            case OperationCode::Id::def_constant:        stream << "def_constant       "; break;
            case OperationCode::Id::read_variable:       stream << "read_variable      "; break;
            case OperationCode::Id::write_variable:      stream << "write_variable     "; break;
            case OperationCode::Id::execute:             stream << "execute            "; break;
            case OperationCode::Id::word_index:          stream << "word_index         "; break;
            case OperationCode::Id::push_constant_value: stream << "push_constant_value"; break;
            case OperationCode::Id::jump:                stream << "jump               "; break;
            case OperationCode::Id::jump_if_zero:        stream << "jump_if_zero       "; break;
            case OperationCode::Id::jump_if_not_zero:    stream << "jump_if_not_zero   "; break;
            case OperationCode::Id::jump_target:         stream << "jump_target        "; break;
        }

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const OperationCode& op)
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


    void execute_code(const std::string& name, const ByteCode& code);


    class ScriptWord
    {
        private:
            struct ContextManager
                {
                    ContextManager()  { mark_context(); }
                    ~ContextManager() { release_context(); }
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
            void operator ()()
            {
                ContextManager manager;
                execute_code(name, code);
            }

        public:
            void show(std::ostream& stream, const std::vector<std::string>& inverse_list)
            {
                stream << "Word: " << name << std::endl;

                for (size_t i = 0; i < code.size(); ++i)
                {
                    stream << std::setw(6) << i << " ";

                    if (   (code[i].id == OperationCode::Id::execute)
                        && (is_numeric(code[i].value)))
                    {
                        auto index = as_numeric<int64_t>(code[i].value);

                        stream << code[i].id
                               << " " << inverse_list[index]
                               << std::endl;
                    }
                    else
                    {
                        stream << code[i] << std::endl;
                    }
                }
            }
    };


    void add_word(const std::string& word, std::function<void()> handler, const Location& location,
                  bool is_immediate, bool is_scripted)
    {
        dictionary.insert(word, {
                .is_immediate = is_immediate,
                .is_scripted = is_scripted,
                .handler_index = word_handlers.insert({
                        .name = word,
                        .function = handler,
                        .definition_location = location
                    })
            });
    }


    void add_word(const std::string& word, std::function<void()> handler,
                  const std::filesystem::path& path, size_t line, size_t column,
                  bool is_immediate)
    {
        add_word(word,
                 handler,
                 Location(std::make_shared<std::filesystem::path>(path), line, column),
                 is_immediate,
                 false);
    }


    #define ADD_NATIVE_WORD(NAME, HANDLER) \
        add_word(NAME, HANDLER, __FILE__, __LINE__, 1, false)

    #define ADD_IMMEDIATE_WORD(NAME, HANDLER) \
        add_word(NAME, HANDLER, __FILE__, __LINE__, 1, true)



    void execute_code(const std::string& name, const ByteCode& code)
    {
        if (is_showing_run_code)
        {
            std::cout << "-------------------------------------" << std::endl;
        }

        for (size_t pc = 0; pc < code.size(); ++pc)
        {
            const OperationCode& operation = code[pc];

            if (is_interpreter_quitting)
            {
                break;
            }

            if (is_showing_run_code)
            {
                std::cout << std::setw(6) << pc << " " << operation << std::endl;
            }

            if (operation.location)
            {
                current_location = operation.location.value();
                call_stack_push(name, operation.location.value());
            }

            switch (operation.id)
            {
                case OperationCode::Id::def_variable:
                    {
                        auto name = as_string(operation.value);
                        auto index = variables.insert({});

                        ADD_NATIVE_WORD(name, [index]() { push((int64_t)index); });
                    }
                    break;

                case OperationCode::Id::def_constant:
                    {
                        auto name = as_string(operation.value);
                        auto value = pop();

                        ADD_NATIVE_WORD(name, [value]() { push(value); });
                    }
                    break;

                case OperationCode::Id::read_variable:
                    {
                        auto index = as_numeric<int64_t>(pop());
                        push(variables[index]);
                    }
                    break;

                case OperationCode::Id::write_variable:
                    {
                        auto index = as_numeric<int64_t>(pop());
                        auto value = pop();

                        variables[index] = value;
                    }
                    break;

                case OperationCode::Id::execute:
                    {
                        if (is_string(operation.value))
                        {
                            auto name = as_string(operation.value);
                            auto [found, word] = dictionary.find(name);

                            if (!found)
                            {
                                throw_error(current_location, "Word '" + name + "' not found.");
                            }

                            auto& word_handler = word_handlers[word.handler_index];

                            call_stack_push(word_handler);
                            word_handler.function();
                            call_stack_pop();
                        }
                        else if (is_numeric(operation.value))
                        {
                            auto index = as_numeric<int64_t>(operation.value);
                            auto& word_handler = word_handlers[index];

                            call_stack_push(word_handler);
                            word_handler.function();
                            call_stack_pop();
                        }
                        else
                        {
                            throw_error(current_location, "Can not execute unexpected value type.");
                        }
                    }
                    break;

                case OperationCode::Id::word_index:
                    {
                        auto name = as_string(operation.value);
                        auto [found, word] = dictionary.find(name);

                        if (!found)
                        {
                            throw_error(current_location, "Word '" + name + "' not found.");
                        }

                        push((int64_t)word.handler_index);
                    }
                    break;

                case OperationCode::Id::push_constant_value:
                    push(operation.value);
                    break;

                case OperationCode::Id::jump:
                    pc += as_numeric<int64_t>(operation.value) - 1;
                    break;

                case OperationCode::Id::jump_if_zero:
                    {
                        auto top = pop();
                        auto value = as_numeric<bool>(top);

                        if (!value)
                        {
                            pc += as_numeric<int64_t>(operation.value) - 1;
                        }
                    }
                    break;

                case OperationCode::Id::jump_if_not_zero:
                    {
                        auto top = pop();
                        auto value = as_numeric<bool>(top);

                        if (value)
                        {
                            pc += as_numeric<int64_t>(operation.value) - 1;
                        }
                    }
                    break;

                case OperationCode::Id::jump_target:
                    // Nothing to do here.  This instruction just acts as a landing pad for the
                    // jump instructions.
                    break;
            }

            if (operation.location)
            {
                call_stack_pop();
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
        Location location;
        ByteCode code;
    };

    using ConstructionStack = std::stack<Construction>;
    using ConstructionList = std::vector<Construction>;


    ConstructionStack construction_stack;
    ConstructionList saved_blocks;

    bool user_is_inserting_at_beginning = false;  // True if Forth level code wants to insert newly
                                                  // generated bytecode at the beginning of the
                                                  // current code block.

    size_t current_token;
    bool new_word_is_immediate = false;

    TokenList input_tokens;

    std::stack<std::filesystem::path> current_path;  // Keep track of previous working directories
                                                     // as we change the current one.


    void compile_token(const Token& token)
    {
        // In Forth anything can be a word, so first we see if it's defined in the dictionary.  If
        // it is, we either compile or execute the word depending on if it's an immediate.
        auto [found, word] = token.type != Token::Type::string
                             ? dictionary.find(token.text)
                             : std::tuple<bool, Word>{ false, {} };

        if (found)
        {
            if (word.is_immediate)
            {
                current_location = token.location;
                auto& word_handler = word_handlers[word.handler_index];

                call_stack_push(word_handler);
                word_handler.function();
                call_stack_pop();
            }
            else
            {
                construction_stack.top().code.push_back({
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
                    {
                        // This word wasn't found, so leave it for resolution at run time.
                        construction_stack.top().code.push_back({
                                .id = OperationCode::Id::execute,
                                .value = token.text,
                                .location = token.location
                            });
                    }
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

        execute_code(buffer.current_location().get_path()->filename().string(),
                     construction_stack.top().code);

        construction_stack.pop();
    }


    void process_source(const std::string& source_text)
    {
        SourceBuffer source(source_text);
        process_source(source);
    }


    void process_source(const std::filesystem::path& path)
    {
        auto full_source_path = std::filesystem::canonical(path);

        auto base_path = full_source_path;
        base_path.remove_filename();

        SourceBuffer source(full_source_path);

        struct WdSetter
        {
            WdSetter(const std::filesystem::path& path) { set_wd(path); }
            ~WdSetter() { reset_wd(); }
        };

        WdSetter setter(base_path);
        process_source(source);
    }


    void process_repl()
    {
        std::cout << "Strange Forth REPL." << std::endl
                  << std::endl
                  << "Enter quit, q, or exit to quit the REPL." << std::endl
                  << std::endl;

        while (   (is_interpreter_quitting == false)
               && (std::cin))
        {
            try
            {
                std::cout << std::endl;

                auto [ found, word ] = dictionary.find("prompt");

                if (found)
                {
                    word_handlers[word.handler_index].function();
                }

                std::string line;
                std::getline(std::cin, line);

                process_source(line);

                std::cout << "ok" << std::endl;
            }
            catch (const std::runtime_error& e)
            {
                std::cerr << std::endl
                          << e.what() << std::endl
                          << std::endl
                          << "Call stack:" << std::endl
                          << call_stack << std::endl;
            }
            catch (...)
            {
                std::cerr << std::endl
                          << "Unknown exception." << std::endl
                          << std::endl
                          << "Call stack:" << std::endl
                          << call_stack << std::endl;
            }
        }
    }


    // Helpers for some of the words.

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


    void logic_bit_op(std::function<int64_t(int64_t, int64_t)> op)
    {
        auto b = pop();
        auto a = pop();
        auto result = op(as_numeric<int64_t>(a), as_numeric<int64_t>(b));

        push(result);
    }


    void string_or_numeric_op(std::function<void(double,double)> dop,
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
        else if (either_is<double>(a, b))
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


    // All of the built in words are defined here.

    void word_quit()
    {
        is_interpreter_quitting = true;

        if (!stack.empty())
        {
            auto value = pop();

            if (is_numeric(value))
            {
                exit_code = as_numeric<int64_t>(value);
            }
        }
    }


    void word_reset()
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


    void insert_user_instruction(const OperationCode& code)
    {
        if (!user_is_inserting_at_beginning)
        {
            construction_stack.top().code.push_back(code);
        }
        else
        {
            construction_stack.top().code.insert(construction_stack.top().code.begin(), code);
        }
    }


    void word_def_variable()
    {
        insert_user_instruction({
                .id = OperationCode::Id::def_variable,
                .value = as_string(pop())
            });
    }


    void word_def_constant()
    {
        insert_user_instruction({
                .id = OperationCode::Id::def_constant,
                .value = as_string(pop())
            });
    }


    void word_read_variable()
    {
        insert_user_instruction({
                .id = OperationCode::Id::read_variable,
                .value = 0
            });
    }


    void word_write_variable()
    {
        insert_user_instruction({
                .id = OperationCode::Id::write_variable,
                .value = 0
            });
    }


    void word_execute()
    {
        insert_user_instruction({
                .id = OperationCode::Id::execute,
                .value = pop()
            });
    }


    void word_push_constant_value()
    {
        insert_user_instruction({
                .id = OperationCode::Id::push_constant_value,
                .value = pop()
            });
    }


    void word_jump()
    {
        insert_user_instruction({
                .id = OperationCode::Id::jump,
                .value = pop()
            });
    }


    void word_jump_if_zero()
    {
        insert_user_instruction({
                .id = OperationCode::Id::jump_if_zero,
                .value = pop()
            });
    }


    void word_jump_if_not_zero()
    {
        insert_user_instruction({
                .id = OperationCode::Id::jump_if_not_zero,
                .value = pop()
            });
    }


    void word_jump_target()
    {
        insert_user_instruction({
                .id = OperationCode::Id::jump_target,
                .value = pop()
            });
    }


    void word_new_code_block()
    {
        construction_stack.push({});
    }


    void word_merge_stack_code_block()
    {
        auto top_code = construction_stack.top().code;
        construction_stack.pop();

        construction_stack.top().code.insert(construction_stack.top().code.end(),
                                             top_code.begin(),
                                             top_code.end());
    }


    void word_resolve_code_jumps()
    {
        auto is_jump = [](const OperationCode& code) -> bool
            {
                return    (code.id == OperationCode::Id::jump)
                       || (code.id == OperationCode::Id::jump_if_not_zero)
                       || (code.id == OperationCode::Id::jump_if_zero);
            };

        auto& top_code = construction_stack.top().code;

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
                jump_targets.insert({ as_string(top_code[i].value), i });
                top_code[i].value = 0;
            }
        }

        for (auto jump_index : jump_indicies)
        {
            auto& jump_op = top_code[jump_index];

            auto jump_name = as_string(jump_op.value);
            auto iter = jump_targets.find(jump_name);

            if (iter != jump_targets.end())
            {
                auto target_index = iter->second;
                jump_op.value = (int64_t)target_index - (int64_t)jump_index;
            }
        }
    }


    void word_compile_until_words()
    {
        auto count = as_numeric<int64_t>(pop());
        std::vector<std::string> word_list;

        for (int64_t i = 0; i < count; ++i)
        {
            word_list.push_back(as_string(pop()));
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

        for (++current_token; current_token < input_tokens.size(); ++current_token)
        {
            auto& token = input_tokens[current_token];

            auto [found, word] = is_one_of_words(token.text);

            if ((found) && (token.type == Token::Type::word))
            {
                push(word);
                return;
            }

            compile_token(token);
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

        throw_error(input_tokens[current_token].location, message);
    }


    void word_code_insert_at_front()
    {
        user_is_inserting_at_beginning = as_numeric<bool>(pop());
    }


    void word_word()
    {
        push(input_tokens[++current_token]);
    }


    void word_word_index()
    {
        ++current_token;
        auto name = input_tokens[current_token].text;

        construction_stack.top().code.push_back({
                .id = OperationCode::Id::word_index,
                .value = name
            });
    }


    void word_start_word()
    {
        ++current_token;
        auto& name = input_tokens[current_token].text;
        auto& location = input_tokens[current_token].location;

        construction_stack.push({ .name = name, .location = location });

        new_word_is_immediate = false;
    }


    void word_end_word()
    {
        auto construction = construction_stack.top();
        construction_stack.pop();

        if (is_showing_bytecode)
        {
            std::cerr << "Defined word " << construction.name << std::endl
                      << construction.code << std::endl;
        }

        add_word(construction.name,
                 ScriptWord(construction.name, construction.code, construction.location),
                 construction.location,
                 new_word_is_immediate,
                 true);
    }


    void word_immediate()
    {
        new_word_is_immediate = true;
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

        ADD_NATIVE_WORD(definition_ptr->name + ".new", [definition_ptr]()
            {
                DataObjectPtr new_object = std::make_shared<DataObject>();

                new_object->definition = definition_ptr;
                new_object->fields.resize(definition_ptr->fieldNames.size());

                push(new_object);
            });

        for (int64_t i = 0; i < definition_ptr->fieldNames.size(); ++i)
        {
            ADD_NATIVE_WORD(definition_ptr->name + "." + definition_ptr->fieldNames[i],
                [i]()
                {
                    push(i);
                });
        }
    }


    void word_read_field()
    {
        auto var = pop();

        if (!std::holds_alternative<DataObjectPtr>(var))
        {
            throw_error(current_location, "Expected data object.");
        }

        auto object = std::get<DataObjectPtr>(var);

        var = pop();
        auto field_index = as_numeric<int64_t>(var);

        push(object->fields[field_index]);
    }


    void word_write_field()
    {
        auto var = pop();

        if (!std::holds_alternative<DataObjectPtr>(var))
        {
            throw_error(current_location, "Expected data object.");
        }

        auto object = std::get<DataObjectPtr>(var);

        var = pop();
        auto field_index = as_numeric<int64_t>(var);

        var = pop();
        object->fields[field_index] = var;
    }


    void word_buffer_new()
    {
        auto size = as_numeric<int64_t>(pop());
        auto buffer = std::make_shared<ByteBuffer>(size);

        push(buffer);
    }


    void word_buffer_write_int()
    {
        auto byte_size = as_numeric<int64_t>(pop());
        auto buffer = as_byte_buffer(pop());
        auto value = as_numeric<int64_t>(pop());

        check_buffer_index(buffer, byte_size);
        buffer->write_int(byte_size, value);
    }


    void word_buffer_read_int()
    {
        auto is_signed = as_numeric<bool>(pop());
        auto byte_size = as_numeric<int64_t>(pop());
        auto buffer = as_byte_buffer(pop());

        check_buffer_index(buffer, byte_size);
        push(buffer->read_int(byte_size, is_signed));
    }


    void word_buffer_write_float()
    {
        auto byte_size = as_numeric<int64_t>(pop());
        auto buffer = as_byte_buffer(pop());
        auto value = as_numeric<double>(pop());

        check_buffer_index(buffer, byte_size);
        buffer->write_float(byte_size, value);
    }


    void word_buffer_read_float()
    {
        auto byte_size = as_numeric<int64_t>(pop());
        auto buffer = as_byte_buffer(pop());

        check_buffer_index(buffer, byte_size);
        push(buffer->read_float(byte_size));
    }


    void word_buffer_set_postion()
    {
        auto buffer_value = pop();

        if (!std::holds_alternative<ByteBufferPtr>(buffer_value))
        {
            throw_error(current_location, "Expected a byte buffer.");
        }

        auto buffer = std::get<ByteBufferPtr>(buffer_value);
        auto new_position = as_numeric<int64_t>(pop());

        buffer->set_position(new_position);
    }


    void word_buffer_get_postion()
    {
        auto buffer_value = pop();

        if (!std::holds_alternative<ByteBufferPtr>(buffer_value))
        {
            throw_error(current_location, "Expected a byte buffer.");
        }

        auto buffer = std::get<ByteBufferPtr>(buffer_value);
        auto new_position = as_numeric<int64_t>(pop());

        push(buffer->postion());
    }


    void word_unique_str()
    {
        static int32_t index = 0;

        std::stringstream stream;

        stream << "unique-" << std::setw(4) << std::setfill('0') << std::hex << index;
        push(stream.str());

        ++index;
    }


    void word_add()
    {
        string_or_numeric_op([](auto a, auto b) { push(a + b); },
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


    void word_left_shift()
    {
        logic_bit_op([](auto value, auto amount) { return value << amount; });
    }


    void word_right_shift()
    {
        logic_bit_op([](auto value, auto amount) { return value >> amount; });
    }


    void word_equal()
    {
        string_or_numeric_op([](auto a, auto b) { push((bool)(a == b)); },
                             [](auto a, auto b) { push((bool)(a == b)); },
                             [](auto a, auto b) { push((bool)(a == b)); });

        // TODO: Extend equal to allow structs to be compared.
    }


    void word_not_equal()
    {
        string_or_numeric_op([](auto a, auto b) { push((bool)(a != b)); },
                             [](auto a, auto b) { push((bool)(a != b)); },
                             [](auto a, auto b) { push((bool)(a != b)); });
    }


    void word_greater_equal()
    {
        string_or_numeric_op([](auto a, auto b) { push((bool)(a >= b)); },
                             [](auto a, auto b) { push((bool)(a >= b)); },
                             [](auto a, auto b) { push((bool)(a >= b)); });
    }


    void word_less_equal()
    {
        string_or_numeric_op([](auto a, auto b) { push((bool)(a <= b)); },
                             [](auto a, auto b) { push((bool)(a <= b)); },
                             [](auto a, auto b) { push((bool)(a <= b)); });
    }


    void word_greater()
    {
        string_or_numeric_op([](auto a, auto b) { push((bool)(a > b)); },
                             [](auto a, auto b) { push((bool)(a > b)); },
                             [](auto a, auto b) { push((bool)(a > b)); });
    }


    void word_less()
    {
        string_or_numeric_op([](auto a, auto b) { push((bool)(a < b)); },
                             [](auto a, auto b) { push((bool)(a < b)); },
                             [](auto a, auto b) { push((bool)(a < b)); });
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


    void word_exit_success()
    {
        push(EXIT_SUCCESS);
    }


    void word_exit_failure()
    {
        push(EXIT_FAILURE);
    }


    void word_true()
    {
        push(true);
    }


    void word_false()
    {
        push(false);
    }


    void word_print()
    {
        std::cout << pop() << " ";
    }


    void word_print_nl()
    {
        std::cout << std::endl;
    }


    void word_hex()
    {
        auto int_value = as_numeric<int64_t>(pop());

        std::cout << std::hex << int_value << std::dec << " ";
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
        std::cout << dictionary;
    }


    void word_show_bytecode()
    {
        auto enable = as_numeric<bool>(pop());

        is_showing_bytecode = enable;
    }


    void word_show_run_code()
    {
        auto enable = as_numeric<bool>(pop());

        is_showing_run_code = enable;
    }


    void word_show_word()
    {
        ++current_token;

        auto name = input_tokens[current_token].text;
        auto [ found, word ] = dictionary.find(name);

        if (!found)
        {
            std::cerr << "Word " << name << " has not been defined." << std::endl;
            return;
        }

        if (word.is_scripted)
        {
            auto& handler = word_handlers[word.handler_index].function;
            auto script_handler = handler.target<ScriptWord>();

            auto inverse_list = inverse_lookup_list(dictionary, word_handlers);

            script_handler->show(std::cout, inverse_list);
        }
        else
        {
            std::cout << "Word " << name << " is a native function." << std::endl;
        }
    }


    // Gather up all the native words and make them available to the interpreter.

    void init_builtin_words()
    {
        // Words for changing interpreter state.
        ADD_NATIVE_WORD("quit", word_quit);
        ADD_NATIVE_WORD("reset", word_reset);
        ADD_NATIVE_WORD("include", word_include);

        // Words for compiling new bytecode.
        ADD_NATIVE_WORD("op.def_variable", word_def_variable);
        ADD_NATIVE_WORD("op.def_constant", word_def_constant);
        ADD_NATIVE_WORD("op.read_variable", word_read_variable);
        ADD_NATIVE_WORD("op.write_variable", word_write_variable);
        ADD_NATIVE_WORD("op.execute", word_execute);
        ADD_NATIVE_WORD("op.push_constant_value", word_push_constant_value);
        ADD_NATIVE_WORD("op.jump", word_jump);
        ADD_NATIVE_WORD("op.jump_if_zero", word_jump_if_zero);
        ADD_NATIVE_WORD("op.jump_if_not_zero", word_jump_if_not_zero);
        ADD_NATIVE_WORD("op.jump_target", word_jump_target);

        ADD_NATIVE_WORD("code.new_block", word_new_code_block);
        ADD_NATIVE_WORD("code.merge_stack_block", word_merge_stack_code_block);
        ADD_NATIVE_WORD("code.resolve_jumps", word_resolve_code_jumps);
        ADD_NATIVE_WORD("code.compile_until_words", word_compile_until_words);
        ADD_NATIVE_WORD("code.insert_at_front", word_code_insert_at_front);

        ADD_NATIVE_WORD("word", word_word);
        ADD_IMMEDIATE_WORD("`", word_word_index);

        // Creating new words.
        ADD_IMMEDIATE_WORD(":", word_start_word);
        ADD_IMMEDIATE_WORD(";", word_end_word);
        ADD_IMMEDIATE_WORD("immediate", word_immediate);

        // Data structure support.
        ADD_IMMEDIATE_WORD("#", word_data_definition);
        ADD_NATIVE_WORD("#@", word_read_field);
        ADD_NATIVE_WORD("#!", word_write_field);

        // ByteBuffer operations.
        ADD_NATIVE_WORD("buffer.new", word_buffer_new);

        ADD_NATIVE_WORD("buffer.int!", word_buffer_write_int);
        ADD_NATIVE_WORD("buffer.int@", word_buffer_read_int);

        ADD_NATIVE_WORD("buffer.float!", word_buffer_write_float);
        ADD_NATIVE_WORD("buffer.float@", word_buffer_read_float);

        ADD_NATIVE_WORD("buffer.position!", word_buffer_set_postion);
        ADD_NATIVE_WORD("buffer.position@", word_buffer_get_postion);

        // Math ops.
        ADD_NATIVE_WORD("+", word_add);
        ADD_NATIVE_WORD("-", word_subtract);
        ADD_NATIVE_WORD("*", word_multiply);
        ADD_NATIVE_WORD("/", word_divide);

        // Bitwise operator words.
        ADD_NATIVE_WORD("and", word_and);
        ADD_NATIVE_WORD("or", word_or);
        ADD_NATIVE_WORD("xor", word_xor);
        ADD_NATIVE_WORD("not", word_not);
        ADD_NATIVE_WORD("<<", word_left_shift);
        ADD_NATIVE_WORD(">>", word_right_shift);

        // Equality words.
        ADD_NATIVE_WORD("=", word_equal);
        ADD_NATIVE_WORD("<>", word_not_equal);
        ADD_NATIVE_WORD(">=", word_greater_equal);
        ADD_NATIVE_WORD("<=", word_less_equal);
        ADD_NATIVE_WORD(">", word_greater);
        ADD_NATIVE_WORD("<", word_less);

        // Stack words.
        ADD_NATIVE_WORD("dup", word_dup);
        ADD_NATIVE_WORD("drop", word_drop);
        ADD_NATIVE_WORD("swap", word_swap);
        ADD_NATIVE_WORD("over", word_over);
        ADD_NATIVE_WORD("rot", word_rot);

        // Some built in constants.
        ADD_NATIVE_WORD("unique_str", word_unique_str);

        ADD_NATIVE_WORD("exit_success", word_exit_success);
        ADD_NATIVE_WORD("exit_failure", word_exit_failure);
        ADD_NATIVE_WORD("true", word_true);
        ADD_NATIVE_WORD("false", word_false);

        // Debug and printing support.
        ADD_NATIVE_WORD(".", word_print);
        ADD_NATIVE_WORD("cr", word_print_nl);
        ADD_NATIVE_WORD(".hex", word_hex);

        ADD_NATIVE_WORD(".s", word_print_stack);
        ADD_NATIVE_WORD(".w", word_print_dictionary);

        ADD_NATIVE_WORD("show_bytecode", word_show_bytecode);
        ADD_NATIVE_WORD("show_run_code", word_show_run_code);
        ADD_IMMEDIATE_WORD("show_word", word_show_word);
    }


}



int main(int argc, char* argv[])
{
    try
    {
        // Keep track of the current working directory.  We'll be changing it as we load scripts.
        current_path.push(std::filesystem::current_path());

        std::filesystem::path std_path = std::filesystem::canonical(argv[0]).remove_filename()
                                         / "std.f";

        // Initialize the language built-in words.
        init_builtin_words();
        process_source(std_path);

        // Mark the context for easy reset in the repl.
        mark_context();

        if (argc == 2)
        {
            // Looks like we're running a script from the command line.
            std::filesystem::path source_path = argv[1];

            if (!std::filesystem::exists(source_path))
            {
                throw std::runtime_error(std::string("File ") + source_path.string() +
                                         " does not exist.");
            }

            process_source(source_path);
        }
        else
        {
            // Run the repl.
            process_repl();
        }
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << std::endl
                  << e.what() << std::endl
                  << std::endl
                  << "Call stack:" << std::endl
                  << call_stack << std::endl;

        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << std::endl
                  << "Unknown exception." << std::endl
                  << std::endl
                  << "Call stack:" << std::endl
                  << call_stack << std::endl;

        return EXIT_FAILURE;
    }

    return exit_code;
}
