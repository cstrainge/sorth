
#pragma once


namespace sorth
{


    struct SubThreadInfo
    {
        internal::Word word;
        std::shared_ptr<std::thread> word_thread;
        bool thead_deleted;

        BlockingValueQueue inputs;
        BlockingValueQueue outputs;
    };


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

            virtual internal::CodeConstructor& constructor() = 0;

        public:
            virtual internal::Location get_current_location() const = 0;

            virtual bool& showing_run_code() = 0;
            virtual bool& showing_bytecode() = 0;

            virtual void halt() = 0;
            virtual void clear_halt_flag() = 0;

            virtual const internal::CallStack& get_call_stack() const = 0;

            virtual std::tuple<bool, internal::Word> find_word(const std::string& word) = 0;
            virtual internal::WordHandlerInfo& get_handler_info(size_t index) = 0;
            virtual std::vector<std::string> get_inverse_lookup_list() = 0;

            virtual std::list<SubThreadInfo> sub_threads() = 0;

            virtual std::thread::id execute_word_threaded(const internal::Word& word) = 0;

            virtual void execute_word(const std::string& word) = 0;
            virtual void execute_word(const internal::Word& word) = 0;
            virtual void execute_word(const internal::Location& location,
                                      const internal::Word& word) = 0;

            virtual void execute_code(const std::string& name, const internal::ByteCode& code) = 0;

            virtual void print_dictionary(std::ostream& stream) = 0;
            virtual void print_stack(std::ostream& stream) = 0;

            virtual bool is_stack_empty() const = 0;
            virtual void clear_stack() = 0;

            virtual int64_t depth() const = 0;
            virtual void push(const Value& value) = 0;
            virtual Value pop() = 0;

            virtual int64_t thread_input_depth(std::thread::id id) = 0;
            virtual void thread_push_input(std::thread::id& id, Value& value) = 0;
            virtual Value thread_pop_input(std::thread::id& id) = 0;

            virtual int64_t thread_output_depth(std::thread::id id) = 0;
            virtual void thread_push_output(std::thread::id& id, Value& value) = 0;
            virtual Value thread_pop_output(std::thread::id& id) = 0;

            virtual Value pick(int64_t index) = 0;
            virtual void push_to(int64_t index) = 0;

        public:
            virtual VariableList& get_variables() = 0;
            virtual const internal::Dictionary& get_dictionary() const = 0;

        public:
            virtual void add_word(const std::string& word,
                                  internal::WordFunction handler,
                                  const internal::Location& location,
                                  bool is_immediate = false,
                                  bool is_hidden = false,
                                  bool is_scripted = false,
                                  const std::string& description = "",
                                  const std::string& signature = "") = 0;

            virtual void add_word(const std::string& word, internal::WordFunction handler,
                                  const std::filesystem::path& path, size_t line, size_t column,
                                  bool is_immediate, const std::string& description = "",
                                  const std::string& signature = "") = 0;

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
    InterpreterPtr clone_interpreter(InterpreterPtr& interpreter);


    #define ADD_NATIVE_WORD(INTERPRETER, NAME, HANDLER, DESCRIPTION, SIGNATURE) \
        INTERPRETER->add_word(NAME, HANDLER, __FILE__, __LINE__, 1, false, DESCRIPTION, SIGNATURE)

    #define ADD_IMMEDIATE_WORD(INTERPRETER, NAME, HANDLER, DESCRIPTION, SIGNATURE) \
        INTERPRETER->add_word(NAME, HANDLER, __FILE__, __LINE__, 1, true, DESCRIPTION, SIGNATURE)


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
            return (variant)std::get<bool>(value);
        }

        if (std::holds_alternative<int64_t>(value))
        {
            return (variant)std::get<int64_t>(value);
        }

        if (std::holds_alternative<double>(value))
        {
            return (variant)std::get<double>(value);
        }

        internal::throw_error(*interpreter, "Expected numeric or boolean value.");
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

        internal::throw_error(*interpreter, "No string conversion for value.");
    }


    inline ByteBufferPtr as_byte_buffer(InterpreterPtr interpreter, Value value)
    {
        if (!std::holds_alternative<ByteBufferPtr>(value))
        {
            internal::throw_error(*interpreter, "Expected a byte buffer.");
        }

        return std::get<ByteBufferPtr>(value);
    }


    inline HashTablePtr as_hash_table(InterpreterPtr interpreter, Value value)
    {
        if (!std::holds_alternative<HashTablePtr>(value))
        {
            internal::throw_error(*interpreter, "Expected a hash table.");
        }

        return std::get<HashTablePtr>(value);
    }


    inline ArrayPtr as_array(InterpreterPtr& interpreter, const Value& value)
    {
        internal::throw_error_if(!std::holds_alternative<ArrayPtr>(value),
                                 *interpreter,
                                 "Expected an array object.");

        return std::get<ArrayPtr>(value);
    }

}
