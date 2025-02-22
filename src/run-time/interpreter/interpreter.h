
#pragma once


namespace sorth
{


    // What mode are we running scripts in?
    enum class ExecutionMode
    {
        // Run the script in the interpreter.
        byte_code,

        // JIT compile the script and run it.
        jit
    };


    namespace internal
    {


        // The name and location of an item on the call stack.
        struct CallItem
        {
            Location word_location;
            std::string word_name;
        };


        // Keep track of the stack of called words.
        using CallStack = std::list<CallItem>;


        std::ostream& operator <<(std::ostream& stream, const CallStack& call_stack);


        // The list of variables that are currently in scope in the interpreter.
        using VariableList = ContextualList<Value>;


        struct WordHandlerInfo
        {
            std::string name;
            WordFunction function;
            Location definition_location;
        };

        using WordList = ContextualList<WordHandlerInfo>;


    }




    // Data structure that keeps track of interpreter sub-threads.
    struct SubThreadInfo
    {
        // The word that the thread is executing.
        internal::Word word;

        // The thread that is executing the word.
        std::shared_ptr<std::thread> word_thread;

        // Has the thread been marked for deletion?
        bool thead_deleted;


        // The thread's input and output queues.
        BlockingValueQueue inputs;
        BlockingValueQueue outputs;
    };


    class SORTH_API Interpreter
    {
        public:
            Interpreter()
            {
            }

            virtual ~Interpreter()
            {
            }

        public:
            virtual ExecutionMode get_execution_mode() const = 0;

        public:
            virtual void mark_context() = 0;
            virtual void release_context() = 0;

        public:
            virtual void process_source(const std::filesystem::path& path) = 0;
            virtual void process_source(const std::string& name,
                                        const std::string& source_text) = 0;

            virtual int get_exit_code() const = 0;
            virtual void set_exit_code(int exit_code) = 0;

            virtual internal::CompileContext& compile_context() = 0;

        public:
            virtual internal::Location get_current_location() const = 0;
            virtual void set_location(const internal::Location& location) = 0;

            virtual bool& showing_run_code() = 0;
            virtual bool& showing_bytecode() = 0;

            virtual void halt() = 0;
            virtual void clear_halt_flag() = 0;

            virtual const internal::CallStack& get_call_stack() const = 0;

            virtual void call_stack_push(const std::string& name,
                                         const internal::Location& location) = 0;
            virtual void call_stack_push(const internal::WordHandlerInfo& handler_info) = 0;
            virtual void call_stack_pop() = 0;

            virtual std::tuple<bool, internal::Word> find_word(const std::string& word) = 0;
            virtual internal::WordHandlerInfo& get_handler_info(size_t index) = 0;

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
            virtual void push_integer(int64_t value) = 0;
            virtual void push_size(size_t value) = 0;
            virtual void push_float(double value) = 0;
            virtual void push_bool(bool value) = 0;
            virtual void push_string(const std::string& value) = 0;
            virtual void push_thread_id(const std::thread::id& value) = 0;

            virtual Value pop() = 0;
            virtual int64_t pop_as_integer() = 0;
            virtual size_t pop_as_size() = 0;
            virtual double pop_as_float() = 0;
            virtual bool pop_as_bool() = 0;
            virtual std::string pop_as_string() = 0;
            virtual std::thread::id pop_as_thread_id() = 0;
            virtual DataObjectPtr pop_as_structure() = 0;
            virtual ArrayPtr pop_as_array() = 0;
            virtual HashTablePtr pop_as_hash_table() = 0;
            virtual ByteBufferPtr pop_as_byte_buffer() = 0;
            virtual internal::Token pop_as_token() = 0;
            virtual internal::ByteCode pop_as_byte_code() = 0;

            virtual int64_t thread_input_depth(std::thread::id id) = 0;
            virtual void thread_push_input(std::thread::id& id, Value& value) = 0;
            virtual Value thread_pop_input(std::thread::id& id) = 0;

            virtual int64_t thread_output_depth(std::thread::id id) = 0;
            virtual void thread_push_output(std::thread::id& id, Value& value) = 0;
            virtual Value thread_pop_output(std::thread::id& id) = 0;

            virtual Value pick(int64_t index) = 0;
            virtual void push_to(int64_t index) = 0;

        public:
            virtual internal::VariableList& get_variables() = 0;
            virtual const internal::Dictionary& get_dictionary() const = 0;

        public:
            virtual void define_variable(const std::string& name) = 0;
            virtual void define_constant(const std::string& name, const Value& value) = 0;
            virtual Value read_variable(size_t index) = 0;
            virtual void write_variable(size_t index, Value value) = 0;

            virtual void add_word(
                          const std::string& word,
                          internal::WordFunction handler,
                          const internal::Location& location,
                          internal::ExecutionContext context = internal::ExecutionContext::run_time,
                          internal::WordVisibility visibility = internal::WordVisibility::visible,
                          internal::WordType type = internal::WordType::internal,
                          const std::string& description = "",
                          const std::string& signature = "") = 0;

            virtual void add_word(const std::string& word,
                                  internal::WordFunction handler,
                                  const std::filesystem::path& path,
                                  size_t line,
                                  size_t column,
                                  internal::ExecutionContext context,
                                  const std::string& description = "",
                                  const std::string& signature = "") = 0;

            virtual void replace_word(const std::string& word, internal::WordFunction handler) = 0;

        public:
            virtual void add_search_path(const std::filesystem::path& path) = 0;
            virtual void add_search_path(const std::string& path) = 0;
            virtual void add_search_path(const char* path) = 0;

            virtual std::filesystem::path find_file(std::filesystem::path& path) = 0;
            virtual std::filesystem::path find_file(const std::string& path) = 0;
            virtual std::filesystem::path find_file(const char* path) = 0;
    };


    using InterpreterPtr = std::shared_ptr<Interpreter>;


    SORTH_API InterpreterPtr create_interpreter(ExecutionMode mode);
    SORTH_API InterpreterPtr clone_interpreter(InterpreterPtr& interpreter);


    #define ADD_NATIVE_WORD(INTERPRETER, NAME, HANDLER, DESCRIPTION, SIGNATURE) \
        INTERPRETER->add_word(NAME, \
                              sorth::internal::WordFunction::Handler(HANDLER), \
                              __FILE__, \
                              __LINE__, \
                              1, \
                              sorth::internal::ExecutionContext::run_time, \
                              DESCRIPTION, \
                              SIGNATURE)

    #define ADD_NATIVE_IMMEDIATE_WORD(INTERPRETER, NAME, HANDLER, DESCRIPTION, SIGNATURE) \
        INTERPRETER->add_word(NAME, \
                              sorth::internal::WordFunction::Handler(HANDLER), \
                              __FILE__, \
                              __LINE__, \
                              1, \
                              sorth::internal::ExecutionContext::compile_time, \
                              DESCRIPTION, \
                              SIGNATURE)


}
