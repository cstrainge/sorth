
#include "sorth.h"



namespace sorth
{


    namespace internal
    {


        std::ostream& operator <<(std::ostream& stream, const CallStack& call_stack)
        {
            stream << std::endl;

            for (const auto& item : call_stack)
            {
                stream << "    " << item.word_location << " -- " << item.word_name << std::endl;
            }

            return stream;
        }


    }


    using namespace internal;


    using ThreadMap = std::unordered_map<std::thread::id, SubThreadInfo>;


    using ValueStack = std::list<Value>;


    namespace
    {


        using PathList = std::list<std::filesystem::path>;


        class InterpreterImpl : public Interpreter,
                                public std::enable_shared_from_this<Interpreter>
        {
            private:
                std::shared_ptr<Interpreter> parent_interpreter;

            private:
                ExecutionMode execution_mode;

                PathList search_paths;

                bool is_interpreter_quitting;
                int exit_code;

                bool is_showing_bytecode;
                bool is_showing_run_code;

                ValueStack stack;

                Location current_location;
                CallStack call_stack;

                Dictionary dictionary;
                WordList word_handlers;

                VariableList variables;

                ThreadMap thread_map;
                std::mutex sub_thread_lock;

                CompileContextStack compile_contexts;

            public:
                InterpreterImpl(ExecutionMode mode);
                InterpreterImpl(InterpreterImpl& interpreter);
                virtual ~InterpreterImpl() override;

            public:
                virtual ExecutionMode get_execution_mode() const override;

            public:
                virtual void mark_context() override;
                virtual void release_context() override;

            public:
                void process_source(SourceBuffer& buffer);

                virtual void process_source(const std::filesystem::path& path) override;
                virtual void process_source(const std::string& name,
                                            const std::string& source_text) override;

                virtual int get_exit_code() const override;
                virtual void set_exit_code(int new_exit_code) override;

                virtual CompileContext& compile_context() override;

            public:
                virtual Location get_current_location() const override;
                virtual void set_location(const internal::Location& location) override;

                virtual bool& showing_run_code() override;
                virtual bool& showing_bytecode() override;

                virtual void halt() override;
                virtual void clear_halt_flag() override;

                virtual const CallStack& get_call_stack() const override;

                virtual void call_stack_push(const std::string& name,
                                             const Location& location) override;
                virtual void call_stack_push(const WordHandlerInfo& handler_info) override;
                virtual void call_stack_pop() override;

                virtual std::tuple<bool, Word> find_word(const std::string& word) override;
                virtual WordHandlerInfo& get_handler_info(size_t index) override;
                virtual std::vector<std::string> get_inverse_lookup_list() override;

            private:
                void append_new_thread(const SubThreadInfo& info);
                void remove_thread(const std::thread::id& id);

            public:
                virtual std::list<SubThreadInfo> sub_threads() override;

                virtual std::thread::id execute_word_threaded(const internal::Word& word) override;

                virtual void execute_word(const std::string& word) override;
                virtual void execute_word(const internal::Word& word) override;
                virtual void execute_word(const Location& location, const Word& word) override;

                virtual void execute_code(const std::string& name, const ByteCode& code) override;

                virtual void print_dictionary(std::ostream& stream) override;
                virtual void print_stack(std::ostream& stream) override;

                virtual bool is_stack_empty() const override;
                virtual void clear_stack() override;

                virtual int64_t depth() const override;
                virtual void push(const Value& value) override;

                virtual Value pop() override;
                virtual int64_t pop_as_integer() override;
                virtual size_t pop_as_size() override;
                virtual double pop_as_float() override;
                virtual bool pop_as_bool() override;
                virtual std::string pop_as_string() override;
                virtual std::thread::id pop_as_thread_id() override;
                virtual DataObjectPtr pop_as_structure() override;
                virtual ArrayPtr pop_as_array() override;
                virtual HashTablePtr pop_as_hash_table() override;
                virtual ByteBufferPtr pop_as_byte_buffer() override;
                virtual Token pop_as_token() override;
                virtual ByteCode pop_as_byte_code() override;

                virtual int64_t thread_input_depth(std::thread::id id) override;
                virtual void thread_push_input(std::thread::id& id, Value& value) override;
                virtual Value thread_pop_input(std::thread::id& id) override;

                virtual int64_t thread_output_depth(std::thread::id id) override;
                virtual void thread_push_output(std::thread::id& id, Value& value) override;
                virtual Value thread_pop_output(std::thread::id& id) override;

            private:
                SubThreadInfo& get_thread_info(const std::thread::id& id);

            public:
                virtual Value pick(int64_t index) override;
                virtual void push_to(int64_t index) override;

            public:
                virtual VariableList& get_variables() override;
                virtual const Dictionary& get_dictionary() const override;

            public:
                virtual void define_variable(const std::string& name) override;
                virtual void define_constant(const std::string& name, const Value& value) override;
                virtual Value read_variable(size_t index) override;
                virtual void write_variable(size_t index, Value value) override;

                virtual void add_word(const std::string& word,
                                      WordFunction handler,
                                      const Location& location,
                                      bool is_immediate = false,
                                      bool is_hidden = false,
                                      bool is_scripted = false,
                                      const std::string& description = "",
                                      const std::string& signature = "") override;

                virtual void add_word(const std::string& word, WordFunction handler,
                                      const std::filesystem::path& path, size_t line, size_t column,
                                      bool is_immediate, const std::string& description,
                                      const std::string& signature) override;

                virtual void replace_word(const std::string& word, WordFunction handler) override;

            public:
                virtual void add_search_path(const std::filesystem::path& path) override;
                virtual void add_search_path(const std::string& path) override;
                virtual void add_search_path(const char* path) override;

                void pop_search_path();

                virtual std::filesystem::path find_file(std::filesystem::path& path) override;
                virtual std::filesystem::path find_file(const std::string& path) override;
                virtual std::filesystem::path find_file(const char* path) override;
        };


        InterpreterImpl::InterpreterImpl(ExecutionMode mode)
        : execution_mode(mode),
          parent_interpreter(),
          is_interpreter_quitting(false),
          exit_code(EXIT_SUCCESS),
          is_showing_bytecode(false),
          is_showing_run_code(false)
        {
        }


        InterpreterImpl::InterpreterImpl(InterpreterImpl& interpreter)
        : execution_mode(interpreter.execution_mode),
          parent_interpreter(std::static_pointer_cast<Interpreter>(interpreter.shared_from_this())),
          search_paths(interpreter.search_paths),
          is_interpreter_quitting(false),
          exit_code(0),
          is_showing_bytecode(false),
          is_showing_run_code(false),
          stack(),
          current_location(interpreter.current_location),
          call_stack(),
          dictionary(interpreter.dictionary),
          word_handlers(interpreter.word_handlers),
          variables(interpreter.variables)
        {
            mark_context();
        }


        InterpreterImpl::~InterpreterImpl()
        {
        }


        ExecutionMode InterpreterImpl::get_execution_mode() const
        {
            return execution_mode;
        }


        void InterpreterImpl::mark_context()
        {
            dictionary.mark_context();
            word_handlers.mark_context();
            variables.mark_context();
        }


        void InterpreterImpl::release_context()
        {
            dictionary.release_context();
            word_handlers.release_context();
            variables.release_context();
        }


        void InterpreterImpl::process_source(SourceBuffer& buffer)
        {
            // Make sure that the compiler context is properly created and freed.
            class CompileContextManager
            {
                private:
                    InterpreterImpl& interpreter;

                public:
                    CompileContextManager(InterpreterImpl& interpreter, TokenList&& tokens)
                    : interpreter(interpreter)
                    {
                        InterpreterPtr shared_this = interpreter.shared_from_this();

                        interpreter.compile_contexts.push(CompileContext(shared_this,
                                                                         std::move(tokens)));
                    }

                    ~CompileContextManager()
                    {
                        interpreter.compile_contexts.pop();
                    }
            };

            // Make sure that the call stack is properly managed.
            class CallStackManager
            {
                private:
                    InterpreterImpl& interpreter;

                public:
                    CallStackManager(InterpreterImpl& interpreter, const std::string& name)
                    : interpreter(interpreter)
                    {
                        interpreter.call_stack_push(name, Location(name, 1, 1));
                    }

                    ~CallStackManager()
                    {
                        interpreter.call_stack_pop();
                    }
            };


            // Now byte-code compile the script.  If we are also JITing the script, then we will
            // cache the non-immediate words for JIT compilation later as a whole module to allow
            // for greater optimization.
            CompileContextManager compile_context_manager(*this, std::move(tokenize(buffer)));

            compile_contexts.top().compile_token_list();

            // Now that the script has been compiled, get the byte-code for the script's top level
            // code.
            auto code = std::move(compile_contexts.top().construction().code);

            // Get the name of the script we are processing.
            auto name = buffer.current_location().get_path();

            #ifndef SORTH_JIT_DISABLED
                if (execution_mode == ExecutionMode::jit)
                {
                    // Get a shared pointer to ourselves.
                    auto this_ptr = shared_from_this();

                    // JIT compile the script's top level function handler and all of the script's
                    // non-immediate words that have been cached during the byte-code compilation
                    // phase.
                    auto handler = jit_module(this_ptr,
                                                name,
                                                code,
                                                compile_contexts.top().word_jit_cache);

                    // Now that the script has been JITed, we can execute it.  We don't need to
                    // save the handler as this is a one time deal.
                    CallStackManager call_stack_manager(*this, name);

                    handler(this_ptr);
                }
                else
                {
                    // Execute the byte-code generated for the script's top level code.
                    CallStackManager call_stack_manager(*this, name);

                    execute_code(name, code);
                }
            #else
                // Execute the byte-code generated for the script's top level code.
                CallStackManager call_stack_manager(*this, name);

                execute_code(name, code);
            #endif
        }


        void InterpreterImpl::process_source(const std::filesystem::path& path)
        {
            // Make sure that the search path is properly managed.  If this script happens to
            // include other scripts, we want to make sure that it can find it's relative scripts.
            //
            // But once we are done processing the script, we want to remove the search path, as it
            // only makes sense in context to the parent script.
            class SearchPathManager
            {
                private:
                    InterpreterImpl& interpreter;

                public:
                    SearchPathManager(InterpreterImpl& interpreter,
                                      const std::filesystem::path& path)
                    : interpreter(interpreter)
                    {
                        interpreter.add_search_path(path);
                    }

                    ~SearchPathManager()
                    {
                        interpreter.pop_search_path();
                    }
            };


            // Get the base directory of the script we're loading so that we can use it as a search
            // path for any relative scripts that may be included from this one..
            auto full_source_path = path;

            auto base_path = full_source_path;
            base_path.remove_filename();

            // Load the script and update the search paths.
            SourceBuffer source(full_source_path);
            SearchPathManager search_path_manager(*this, base_path);

            // Now compile/execute the script.
            process_source(source);
        }


        void InterpreterImpl::process_source(const std::string& name,
                                             const std::string& source_text)
        {
            // Create a source buffer for this script and then compile/execute it.
            SourceBuffer source(name, source_text);
            process_source(source);
        }


        int InterpreterImpl::get_exit_code() const
        {
            return exit_code;
        }


        void InterpreterImpl::set_exit_code(int new_exit_code)
        {
            exit_code = new_exit_code;
        }


        CompileContext& InterpreterImpl::compile_context()
        {
            throw_error_if(compile_contexts.size() == 0,
                           shared_from_this(),
                           "The compiler context is unavailable.");
            return compile_contexts.top();
        }


        Location InterpreterImpl::get_current_location() const
        {
            return current_location;
        }

        void InterpreterImpl::set_location(const internal::Location& location)
        {
            current_location = location;
        }

        bool& InterpreterImpl::showing_run_code()
        {
            return is_showing_run_code;
        }


        bool& InterpreterImpl::showing_bytecode()
        {
            return is_showing_bytecode;
        }


        void InterpreterImpl::halt()
        {
            is_interpreter_quitting = true;
        }


        void InterpreterImpl::clear_halt_flag()
        {
            is_interpreter_quitting = false;
        }


        const CallStack& InterpreterImpl::get_call_stack() const
        {
            return call_stack;
        }


        std::tuple<bool, Word> InterpreterImpl::find_word(const std::string& word)
        {
            return dictionary.find(word);
        }


        WordHandlerInfo& InterpreterImpl::get_handler_info(size_t index)
        {
            throw_error_if(index >= word_handlers.size(), shared_from_this(),
                           "Handler index is out of range.");

            return word_handlers[index];
        }


        std::vector<std::string> InterpreterImpl::get_inverse_lookup_list()
        {
            return inverse_lookup_list(dictionary, word_handlers);
        }


        void InterpreterImpl::append_new_thread(const SubThreadInfo& info)
        {
            if (parent_interpreter)
            {
                auto actual = std::reinterpret_pointer_cast<InterpreterImpl>(parent_interpreter);
                actual->append_new_thread(info);
            }
            else
            {
                std::lock_guard<std::mutex> guard(sub_thread_lock);
                thread_map.insert(ThreadMap::value_type(info.word_thread->get_id(), info));
            }
        }


        void InterpreterImpl::remove_thread(const std::thread::id& id)
        {
            if (parent_interpreter)
            {
                auto actual = std::reinterpret_pointer_cast<InterpreterImpl>(parent_interpreter);
                actual->remove_thread(id);
            }
            else
            {
                auto& info = get_thread_info(id);

                if (info.outputs.depth() == 0)
                {
                    std::lock_guard<std::mutex> guard(sub_thread_lock);

                    auto thread = thread_map[id];

                    thread.word_thread->join();

                    if (thread_map.contains(id))
                    {
                        thread_map.erase(id);
                    }
                }
                else
                {
                    std::lock_guard<std::mutex> guard(sub_thread_lock);

                    info.thead_deleted = true;
                }
            }
        }


        std::list<SubThreadInfo> InterpreterImpl::sub_threads()
        {
            if (parent_interpreter)
            {
                auto actual = std::reinterpret_pointer_cast<InterpreterImpl>(parent_interpreter);
                return actual->sub_threads();
            }

            std::lock_guard<std::mutex> guard(sub_thread_lock);
            std::list<SubThreadInfo> copy;

            for (auto& item : thread_map)
            {
                copy.push_back(item.second);
            }

            return copy;
        }


        std::thread::id InterpreterImpl::execute_word_threaded(const internal::Word& word)
        {
            // If this interpreter has a parent, request it to spawn the thread so that they are
            // all tracked in the same place.
            if (parent_interpreter)
            {
                return parent_interpreter->execute_word_threaded(word);
            }

            // Capture the pointer to this interpreter then clone itself to run in the sub
            // thread.
            auto this_ptr = std::static_pointer_cast<Interpreter>(shared_from_this());
            auto child_interpreter = clone_interpreter(this_ptr);

            // Spawn the new thread.
            auto word_thread = std::make_shared<std::thread>([=]()
                {
                    try
                    {
                        // Get a proper reference to the sub-thread interpreter.
                        auto child =
                              std::reinterpret_pointer_cast<InterpreterImpl>(child_interpreter);

                        // Execute the requested word.
                        child->execute_word(word);

                        // Before we exit clear up the thread reference.
                        child->remove_thread(std::this_thread::get_id());
                    }
                    catch (...)
                    {
                        // TODO: Catch the exception and possibly store that with the thread
                        //       info.
                    }
                });

            // Register the thread.
            append_new_thread(
                {
                    .word = word,
                    .word_thread = word_thread,
                    .thead_deleted = false
                });

            return word_thread->get_id();
        }


        void InterpreterImpl::execute_word(const std::string& word)
        {
            auto [ found, word_entry ] = dictionary.find(word);

            throw_error_if(!found, shared_from_this(), "Word " + word + " was not found.");

            auto this_ptr = shared_from_this();
            word_handlers[word_entry.handler_index].function(this_ptr);
        }


        void InterpreterImpl::execute_word(const internal::Word& word)
        {
            execute_word(current_location, word);
        }


        void InterpreterImpl::execute_word(const Location& location, const Word& word)
        {
            throw_error_if(word.handler_index >= word_handlers.size(),
                           shared_from_this(),
                           "Bad word handler index.");

            current_location = location;
            auto& word_handler = word_handlers[word.handler_index];
            auto this_ptr = shared_from_this();

            call_stack_push(word_handler);

            try
            {
                word_handler.function(this_ptr);
                call_stack_pop();
            }
            catch (...)
            {
                call_stack_pop();
                throw;
            }
        }


        void InterpreterImpl::execute_code(const std::string& name, const ByteCode& code)
        {
            // Keep track of any contexts that get marked so that we can safely clean up if any
            // releases are missed.
            size_t contexts = 0;

            auto cleanup_contexts = [this, &contexts](bool throw_exception)
                {
                    // Make sure we have ballance...
                    for (size_t i = 0; i < contexts; ++i)
                    {
                        release_context();
                    }

                    // Detect and report an error if we're not already in a cleanup context.
                    if ((throw_exception) && (contexts > 0))
                    {
                        throw_error(shared_from_this(), "Unbalanced context handling detected.");
                    }

                    // Otherwise clear up the marker.
                    contexts = 0;
                };

            // Showing debug information?
            if (is_showing_run_code)
            {
                std::cout << "-------[ " << name << " ]------------------------------" << std::endl;
            }

            // Keep track if the current location has been included in the current instruction and
            // the call stack accordingly updated.
            bool call_stack_pushed = false;

            // Keep track of any try/catch blocks and loops.
            std::vector<int64_t> catch_locations;
            std::vector<std::pair<int64_t, int64_t>> loop_locations;

            for (size_t pc = 0; pc < code.size(); ++pc)
            {
                try
                {
                    const Instruction& operation = code[pc];

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

                        call_stack_pushed = true;
                    }

                    switch (operation.id)
                    {
                        case Instruction::Id::def_variable:
                            {
                                auto name = operation.value.as_string(shared_from_this());
                                define_variable(name);
                            }
                            break;

                        case Instruction::Id::def_constant:
                            {
                                auto name = operation.value.as_string(shared_from_this());
                                auto value = pop();

                                define_constant(name, value);
                            }
                            break;

                        case Instruction::Id::read_variable:
                            {
                                auto index = pop_as_size();
                                push(read_variable(index));
                            }
                            break;

                        case Instruction::Id::write_variable:
                            {
                                auto index = pop_as_size();
                                auto value = pop();

                                write_variable(index, value);
                            }
                            break;

                        case Instruction::Id::execute:
                            {
                                if (operation.value.is_string())
                                {
                                    auto name = operation.value.as_string(shared_from_this());
                                    auto [found, word] = dictionary.find(name);

                                    if (!found)
                                    {
                                        throw_error(shared_from_this(), "Word '" + name + "' not found.");
                                    }

                                    auto& word_handler = word_handlers[word.handler_index];

                                    call_stack_push(word_handler);

                                    try
                                    {
                                        auto This = shared_from_this();
                                        word_handler.function(This);

                                    }
                                    catch (...)
                                    {
                                        call_stack_pop();
                                        throw;
                                    }

                                    call_stack_pop();
                                }
                                else if (operation.value.is_numeric())
                                {
                                    auto index = operation.value.as_integer(shared_from_this());
                                    auto& word_handler = word_handlers[index];

                                    call_stack_push(word_handler);

                                    try
                                    {
                                        auto This = shared_from_this();
                                        word_handler.function(This);
                                    }
                                    catch (...)
                                    {
                                        call_stack_pop();
                                        throw;
                                    }

                                    call_stack_pop();
                                }
                                else
                                {
                                    throw_error(shared_from_this(), "Can not execute unexpected value type.");
                                }
                            }
                            break;

                        case Instruction::Id::word_index:
                            {
                                auto name = operation.value.as_string(shared_from_this());
                                auto [ found, word ] = dictionary.find(name);

                                if (!found)
                                {
                                    throw_error(shared_from_this(), "Word '" + name + "' not found.");
                                }

                                push((int64_t)word.handler_index);
                            }
                            break;

                        case Instruction::Id::word_exists:
                            {
                                auto name = operation.value.as_string(shared_from_this());
                                auto [ found, word ] = dictionary.find(name);

                                push(found);
                            }
                            break;

                        case Instruction::Id::push_constant_value:
                            push(operation.value);
                            break;

                        case Instruction::Id::mark_loop_exit:
                            {
                                int64_t relative_jump =
                                                     operation.value.as_integer(shared_from_this());
                                int64_t absolute = pc + relative_jump;

                                loop_locations.push_back({pc + 1, absolute});
                            }
                            break;

                        case Instruction::Id::unmark_loop_exit:
                            throw_error_if(loop_locations.empty(), shared_from_this(),
                                           "Clearing a loop exit without an enclosing loop.");

                            loop_locations.pop_back();
                            break;

                        case Instruction::Id::mark_catch:
                            {
                                int64_t relative_jump =
                                                     operation.value.as_integer(shared_from_this());
                                int64_t absolute = pc + relative_jump;

                                catch_locations.push_back(absolute);
                            }
                            break;

                        case Instruction::Id::unmark_catch:
                            throw_error_if(catch_locations.empty(), shared_from_this(),
                                           "Clearing a catch exit without an enclosing try/catch.");
                            catch_locations.pop_back();
                            break;

                        case Instruction::Id::mark_context:
                            mark_context();
                            ++contexts;
                            break;

                        case Instruction::Id::release_context:
                            if (contexts == 0)
                            {
                                throw_error(shared_from_this(), "Unbalanced context release detected.");
                            }

                            release_context();
                            --contexts;
                            break;

                        case Instruction::Id::jump:
                            pc += operation.value.as_integer(shared_from_this()) - 1;
                            break;

                        case Instruction::Id::jump_if_zero:
                            {
                                auto value = pop_as_bool();

                                if (!value)
                                {
                                    pc += operation.value.as_integer(shared_from_this()) - 1;
                                }
                            }
                            break;

                        case Instruction::Id::jump_if_not_zero:
                            {
                                auto value = pop_as_bool();

                                if (value)
                                {
                                    pc += operation.value.as_integer(shared_from_this()) - 1;
                                }
                            }
                            break;

                        case Instruction::Id::jump_loop_start:
                            if (!loop_locations.empty())
                            {
                                pc = loop_locations.back().first - 1;
                            }
                            break;

                        case Instruction::Id::jump_loop_exit:
                            if (!loop_locations.empty())
                            {
                                pc = loop_locations.back().second - 1;
                                //loop_locations.pop_back();
                            }
                            break;

                        case Instruction::Id::jump_target:
                            // Nothing to do here.  This instruction just acts as a landing pad for
                            // the jump instructions.
                            break;
                    }

                    if (operation.location)
                    {
                        call_stack_pop();
                    }
                }
                catch (const std::runtime_error& error)
                {
                    if (call_stack_pushed)
                    {
                        call_stack_pop();
                        call_stack_pushed = false;
                    }

                    // Check for any catch blocks.
                    if (!catch_locations.empty())
                    {
                        pc = catch_locations.back() - 1;
                        catch_locations.pop_back();
                        push(std::string(error.what()));
                    }
                    else
                    {
                        // No catch block, so clean up any unresolved contexts and rethrow the
                        // exception, we don't need to throw an exception here as this is part of
                        // the cleanup process.
                        cleanup_contexts(false);
                        throw error;
                    }
                }
            }

            // Make sure the context acquisitions are balanced.  Throw an exception if the contexts
            // are unbalanced.
            cleanup_contexts(true);

            // Print debugging information.
            if (is_showing_run_code)
            {
                std::cout << "=======[ " << name << " ]==============================" << std::endl;
            }
        }


        void InterpreterImpl::print_dictionary(std::ostream& stream)
        {
            stream << dictionary << std::endl;
        }


        void InterpreterImpl::print_stack(std::ostream& stream)
        {
            stream << "Stack: " << stack.size() << std::endl;
            for (const auto& value : stack)
            {
                if (value.is_string())
                {
                    stream << stringify(value) << std::endl;
                }
                else
                {
                    stream << value << std::endl;
                }
            }

            stream << "----" << std::endl;
        }


        bool InterpreterImpl::is_stack_empty() const
        {
            return stack.empty();
        }


        void InterpreterImpl::clear_stack()
        {
            stack.clear();
        }


        int64_t InterpreterImpl::depth() const
        {
            return stack.size();
        }


        void InterpreterImpl::push(const Value& value)
        {
            stack.push_front(value);
        }


        Value InterpreterImpl::pop()
        {
            if (stack.empty())
            {
                throw_error(shared_from_this(), "Stack underflow.");
            }

            auto next = stack.front();
            stack.pop_front();

            return next;
        }


        int64_t InterpreterImpl::pop_as_integer()
        {
            return pop().as_integer(shared_from_this());
        }


        size_t InterpreterImpl::pop_as_size()
        {
            return static_cast<size_t>(pop().as_integer(shared_from_this()));
        }


        double InterpreterImpl::pop_as_float()
        {
            return pop().as_float(shared_from_this());
        }


        bool InterpreterImpl::pop_as_bool()
        {
            return pop().as_bool();
        }


        std::string InterpreterImpl::pop_as_string()
        {
            return pop().as_string(shared_from_this());
        }


        std::thread::id InterpreterImpl::pop_as_thread_id()
        {
            return pop().as_thread_id(shared_from_this());
        }


        DataObjectPtr InterpreterImpl::pop_as_structure()
        {
            return pop().as_structure(shared_from_this());
        }


        ArrayPtr InterpreterImpl::pop_as_array()
        {
            return pop().as_array(shared_from_this());
        }


        HashTablePtr InterpreterImpl::pop_as_hash_table()
        {
            return pop().as_hash_table(shared_from_this());
        }


        ByteBufferPtr InterpreterImpl::pop_as_byte_buffer()
        {
            return pop().as_byte_buffer(shared_from_this());
        }


        Token InterpreterImpl::pop_as_token()
        {
            return pop().as_token(shared_from_this());
        }


        ByteCode InterpreterImpl::pop_as_byte_code()
        {
            return pop().as_byte_code(shared_from_this());
        }


        int64_t InterpreterImpl::thread_input_depth(std::thread::id id)
        {
            return get_thread_info(id).inputs.depth();
        }


        void InterpreterImpl::thread_push_input(std::thread::id& id, Value& value)
        {
            get_thread_info(id).inputs.push(value);
        }


        Value InterpreterImpl::thread_pop_input(std::thread::id& id)
        {
            return get_thread_info(id).inputs.pop();
        }


        int64_t InterpreterImpl::thread_output_depth(std::thread::id id)
        {
            return get_thread_info(id).outputs.depth();
        }


        void InterpreterImpl::thread_push_output(std::thread::id& id, Value& value)
        {
            get_thread_info(id).outputs.push(value);
        }


        Value InterpreterImpl::thread_pop_output(std::thread::id& id)
        {
            auto& info = get_thread_info(id);
            auto value = info.outputs.pop();

            // If the thread has exited and we've consumed the last of it's outputs free up the
            // thread record now.
            if (   (info.thead_deleted)
                && (info.outputs.depth() == 0))
            {
                std::lock_guard<std::mutex> guard(sub_thread_lock);

                auto& thread = thread_map[id];

                thread.word_thread->join();
                thread_map.erase(id);
            }

            return value;
        }


        SubThreadInfo& InterpreterImpl::get_thread_info(const std::thread::id& id)
        {
            if (parent_interpreter)
            {
                auto actual = std::reinterpret_pointer_cast<InterpreterImpl>(parent_interpreter);
                return actual->get_thread_info(id);
            }

            std::lock_guard<std::mutex> guard(sub_thread_lock);

            if (!thread_map.contains(id))
            {
                throw_error("Thread id not found.");
            }

            return thread_map[id];
        }


        Value InterpreterImpl::pick(int64_t index)
        {
            auto iterator = stack.begin();

            std::advance(iterator, index);

            auto value = *iterator;

            stack.erase(iterator);

            return value;
        }


        void InterpreterImpl::push_to(int64_t index)
        {
            auto value = pop();
            stack.insert(std::next(stack.begin(), index), value);
        }


        VariableList& InterpreterImpl::get_variables()
        {
            return variables;
        }


        const Dictionary& InterpreterImpl::get_dictionary() const
        {
            return dictionary;
        }


        void InterpreterImpl::define_variable(const std::string& name)
        {
            auto index = variables.insert({});
            auto handler = [index](auto This)
                {
                    This->push((int64_t)index);
                };

            ADD_NATIVE_WORD(this,
                            name,
                            handler,
                            "Access the variable " + name + ".",
                            " -- variable_index");
        }


        void InterpreterImpl::define_constant(const std::string& name, const Value& value)
        {
            ADD_NATIVE_WORD(this,
                            name,
                            [value](InterpreterPtr& This)
                            {
                                This->push(value.deep_copy());
                            },
                            "Constant value " + name + ".",
                            " -- value");
        }


        Value InterpreterImpl::read_variable(size_t index)
        {
            return variables[index];
        }


        void InterpreterImpl::write_variable(size_t index, Value value)
        {
            variables[index] = value;
        }


        void InterpreterImpl::add_word(const std::string& word,
                                       WordFunction handler,
                                       const Location& location,
                                       bool is_immediate,
                                       bool is_hidden,
                                       bool is_scripted,
                                       const std::string& description,
                                       const std::string& signature)
        {
            StringPtr shared_description;

            if (description != "")
            {
                shared_description = std::make_shared<std::string>(description);
            }

            StringPtr shared_signature;

            if (signature != "")
            {
                shared_signature = std::make_shared<std::string>(signature);
            }

            dictionary.insert(word, {
                    .is_immediate = is_immediate,
                    .is_scripted = is_scripted,
                    .is_hidden = is_hidden,
                    .description = shared_description,
                    .signature = shared_signature,
                    .location = location,
                    .handler_index = word_handlers.insert({
                            .name = word,
                            .function = handler,
                            .definition_location = location
                        })
                });
        }


        void InterpreterImpl::add_word(const std::string& word, WordFunction handler,
                                       const std::filesystem::path& path,
                                       size_t line, size_t column,
                                       bool is_immediate,
                                       const std::string& description,
                                       const std::string& signature)
        {
            add_word(word,
                     handler,
                     Location(path.string(), line, column),
                     is_immediate,
                     false,
                     false,
                     description,
                     signature);
        }


        void InterpreterImpl::replace_word(const std::string& word, WordFunction handler)
        {
            auto [ found, word_entry ] = dictionary.find(word);

            if (!found)
            {
                throw_error(shared_from_this(), "Word " + word + " was not found for replacement.");
            }

            auto code_ref = word_handlers[word_entry.handler_index].function.get_byte_code();

            if (code_ref.has_value())
            {
                const auto& byte_code = code_ref.value();
                handler.set_byte_code(byte_code);
            }

            word_handlers[word_entry.handler_index].function = handler;
        }


        void InterpreterImpl::add_search_path(const std::filesystem::path& path)
        {
            if (!path.is_absolute())
            {
                throw std::runtime_error("add_search_path: Path must be absolute.");
            }

            if (!std::filesystem::exists(path))
            {
                throw std::runtime_error("add_search_path: Path must exist, " +
                                         path.string() + ".");
            }

            search_paths.push_front(std::filesystem::canonical(path));
        }


        void InterpreterImpl::add_search_path(const std::string& path)
        {
            std::filesystem::path new_path = path;
            add_search_path(new_path);
        }


        void InterpreterImpl::add_search_path(const char* path)
        {
            std::filesystem::path new_path = path;
            add_search_path(new_path);
        }


        void InterpreterImpl::pop_search_path()
        {
            if (!search_paths.empty())
            {
                search_paths.pop_front();
            }
        }


        std::filesystem::path InterpreterImpl::find_file(std::filesystem::path& path)
        {
            if (!path.is_absolute())
            {
                for (auto next_path : search_paths)
                {
                    auto found_path = next_path / path;

                    if (std::filesystem::exists(found_path))
                    {
                        return found_path;
                    }
                }

                throw std::runtime_error("find_file: File path could not be found, " +
                                        path.string() + ".");
            }

            if (!std::filesystem::exists(path))
            {
                throw std::runtime_error("find_file: File does not exist, " + path.string() + ".");
            }

            return std::filesystem::canonical(path);
        }


        std::filesystem::path InterpreterImpl::find_file(const std::string& path)
        {
            std::filesystem::path new_path = path;
            return find_file(new_path);
        }


        std::filesystem::path InterpreterImpl::find_file(const char* path)
        {
            std::filesystem::path new_path = path;
            return find_file(new_path);
        }


        void InterpreterImpl::call_stack_push(const std::string& name, const Location& location)
        {
            call_stack.push_front({
                    .word_location = location,
                    .word_name = name
                });
        }


        void InterpreterImpl::call_stack_push(const WordHandlerInfo& handler_info)
        {
            call_stack_push(handler_info.name, handler_info.definition_location);
        }


        void InterpreterImpl::call_stack_pop()
        {
            if (!call_stack.empty())
            {
                call_stack.pop_front();
            }
        }


    }


    InterpreterPtr create_interpreter(ExecutionMode mode)
    {
        return std::make_shared<InterpreterImpl>(mode);
    }


    InterpreterPtr clone_interpreter(InterpreterPtr& interpreter)
    {
        InterpreterImpl* raw_ptr = reinterpret_cast<InterpreterImpl*>(&(interpreter));
        return std::make_shared<InterpreterImpl>(*raw_ptr);
    }


}
