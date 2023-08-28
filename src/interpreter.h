
#pragma once


namespace sorth
{


    class Interpreter
    {
        public:
            Interpreter()
            {
            }

            virtual ~Interpreter()
            {
            }

        public:
            virtual void mark_context() = 0;
            virtual void release_context() = 0;

        public:
            virtual void process_source(const std::filesystem::path& path) = 0;
            virtual void process_source(const std::string& source_text) = 0;

            virtual int get_exit_code() const = 0;
            virtual void set_exit_code(int exit_code) = 0;

            virtual internal::OptionalConstructor& constructor() = 0;

        public:
            virtual internal::Location get_current_location() const = 0;

            virtual bool& showing_run_code() = 0;
            virtual bool& showing_bytecode() = 0;

            virtual void halt() = 0;
            virtual void clear_halt_flag() = 0;

            virtual std::tuple<bool, internal::Word> find_word(const std::string& word) = 0;
            virtual internal::WordHandlerInfo& get_handler_info(size_t index) = 0;
            virtual std::vector<std::string> get_inverse_lookup_list() = 0;

            virtual void execute_word(const std::string& word) = 0;
            virtual void execute_word(const internal::Location& location,
                                      const internal::Word& word) = 0;

            virtual void execute_code(const std::string& name, const internal::ByteCode& code) = 0;

            virtual void print_dictionary(std::ostream& stream) = 0;
            virtual void print_stack(std::ostream& stream) = 0;

            virtual bool is_stack_empty() const = 0;
            virtual void push(const Value& value) = 0;
            virtual Value pop() = 0;

        public:
            virtual void add_word(const std::string& word, internal::WordFunction handler,
                                  const internal::Location& location,
                                  bool is_immediate, bool is_scripted) = 0;

            virtual void add_word(const std::string& word, internal::WordFunction handler,
                                  const std::filesystem::path& path, size_t line, size_t column,
                                  bool is_immediate) = 0;

        public:
            virtual void add_search_path(const std::filesystem::path& path) = 0;
            virtual void add_search_path(const std::string& path) = 0;
            virtual void add_search_path(const char* path) = 0;

            virtual std::filesystem::path find_file(std::filesystem::path& path) = 0;
            virtual std::filesystem::path find_file(const std::string& path) = 0;
            virtual std::filesystem::path find_file(const char* path) = 0;
    };


    using InterpreterPtr = std::shared_ptr<Interpreter>;


    InterpreterPtr create_interpreter();


    #define ADD_NATIVE_WORD(INTERPRETER, NAME, HANDLER) \
        INTERPRETER->add_word(NAME, HANDLER, __FILE__, __LINE__, 1, false)

    #define ADD_IMMEDIATE_WORD(INTERPRETER, NAME, HANDLER) \
        INTERPRETER->add_word(NAME, HANDLER, __FILE__, __LINE__, 1, true)


    // Value interrogation and conversion functions.  Because the value itself is a variant, it's
    // helpful to have functions that will handle extraction into native types according to the
    // semantics of the language being implemented.

    template <typename variant>
    inline bool either_is(const Value& a, const Value& b)
    {
        return    std::holds_alternative<variant>(a)
               || std::holds_alternative<variant>(b);
    }


    inline bool is_numeric(const Value& value)
    {
        return    std::holds_alternative<bool>(value)
               || std::holds_alternative<int64_t>(value)
               || std::holds_alternative<double>(value);
    }


    inline bool is_string(const Value& value)
    {
        return    std::holds_alternative<std::string>(value)
               || std::holds_alternative<internal::Token>(value);
    }


    template <typename variant>
    inline variant as_numeric(InterpreterPtr interpreter, const Value& value)
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

        throw_error(interpreter->get_current_location(), "Expected numeric or boolean value.");
    }


    inline std::string as_string(InterpreterPtr interpreter, const Value& value)
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

        if (std::holds_alternative<internal::Token>(value))
        {
            return std::get<internal::Token>(value).text;
        }

        if (std::holds_alternative<std::string>(value))
        {
            return std::get<std::string>(value);
        }

        throw_error(interpreter->get_current_location(), "No string conversion for value.");
    }


    inline ByteBufferPtr as_byte_buffer(InterpreterPtr interpreter, Value value)
    {
        if (!std::holds_alternative<ByteBufferPtr>(value))
        {
            throw_error(interpreter->get_current_location(), "Expected a byte buffer.");
        }

        return std::get<ByteBufferPtr>(value);
    }


}
