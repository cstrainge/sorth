
#include "sorth.h"


namespace sorth
{


    using namespace internal;


    namespace
    {


        using PathList = std::list<std::filesystem::path>;


        class InterpreterImpl : public Interpreter,
                                public std::enable_shared_from_this<Interpreter>
        {
            private:
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

                OptionalConstructor code_constructor;

            public:
                InterpreterImpl();
                virtual ~InterpreterImpl() override;

            public:
                virtual void mark_context() override;
                virtual void release_context() override;

            public:
                void process_source(SourceBuffer& buffer);

                virtual void process_source(const std::filesystem::path& path) override;
                virtual void process_source(const std::string& source_text) override;

                virtual int get_exit_code() const override;
                virtual void set_exit_code(int new_exit_code) override;

                virtual OptionalConstructor& constructor() override;

            public:
                virtual Location get_current_location() const override;

                virtual bool& showing_run_code() override;
                virtual bool& showing_bytecode() override;

                virtual void halt() override;
                virtual void clear_halt_flag() override;

                virtual const CallStack& get_call_stack() const override;

                virtual std::tuple<bool, Word> find_word(const std::string& word) override;
                virtual WordHandlerInfo& get_handler_info(size_t index) override;
                virtual std::vector<std::string> get_inverse_lookup_list() override;

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

                virtual Value pick(int64_t index) override;
                virtual void push_to(int64_t index) override;

            public:
                virtual VariableList& get_variables() override;
                virtual const Dictionary& get_dictionary() const override;

            public:
                virtual void add_word(const std::string& word,
                                      WordFunction handler,
                                      const internal::Location& location,
                                      bool is_immediate,
                                      bool is_hidden,
                                      bool is_scripted,
                                      const std::string& description,
                                      const std::string& signature) override;

                virtual void add_word(const std::string& word, WordFunction handler,
                                      const std::filesystem::path& path, size_t line, size_t column,
                                      bool is_immediate, const std::string& description,
                                      const std::string& signature) override;

            public:
                virtual void add_search_path(const std::filesystem::path& path) override;
                virtual void add_search_path(const std::string& path) override;
                virtual void add_search_path(const char* path) override;

                void pop_search_path();

                virtual std::filesystem::path find_file(std::filesystem::path& path) override;
                virtual std::filesystem::path find_file(const std::string& path) override;
                virtual std::filesystem::path find_file(const char* path) override;

            private:
                void call_stack_push(const std::string& name, const Location& location);
                void call_stack_push(const WordHandlerInfo& handler_info);
                void call_stack_pop();
        };


        InterpreterImpl::InterpreterImpl()
        : is_interpreter_quitting(false),
          exit_code(EXIT_SUCCESS),
          is_showing_bytecode(false),
          is_showing_run_code(false)
        {
            code_constructor = CodeConstructor {};
        }


        InterpreterImpl::~InterpreterImpl()
        {
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
            code_constructor->interpreter = shared_from_this();
            code_constructor->stack.push({});
            code_constructor->input_tokens = tokenize(buffer);
            code_constructor->compile_token_list();

            auto code = code_constructor->stack.top().code;

            auto name = buffer.current_location().get_path()->filename().string();

            if (is_showing_bytecode)
            {
                std::cout << "--------[" << name << "]-------------" << std::endl
                          << code << std::endl;
            }

            execute_code(name, code);

            code_constructor->stack.pop();
        }


        void InterpreterImpl::process_source(const std::filesystem::path& path)
        {
            auto full_source_path = path;

            auto base_path = full_source_path;
            base_path.remove_filename();

            SourceBuffer source(full_source_path);

            if (base_path != "")
            {
                add_search_path(base_path);
            }

            try
            {
                process_source(source);
                pop_search_path();
            }
            catch (...)
            {
                pop_search_path();
                throw;
            }
        }


        void InterpreterImpl::process_source(const std::string& source_text)
        {
            SourceBuffer source(source_text);
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


        OptionalConstructor& InterpreterImpl::constructor()
        {
            throw_error_if(!code_constructor, *this, "Code constructor is unavailable.");
            return code_constructor;
        }


        Location InterpreterImpl::get_current_location() const
        {
            return current_location;
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
            throw_error_if(index >= word_handlers.size(), *this,
                           "Handler index is out of range.");

            return word_handlers[index];
        }


        std::vector<std::string> InterpreterImpl::get_inverse_lookup_list()
        {
            return inverse_lookup_list(dictionary, word_handlers);
        }


        void InterpreterImpl::execute_word(const std::string& word)
        {
            auto [ found, word_entry ] = dictionary.find(word);

            throw_error_if(!found, *this, "Word " + word + " was not found.");

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
                           *this,
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
            if (is_showing_run_code)
            {
                std::cout << "-------[ " << name << " ]------------------------------" << std::endl;
            }

            bool call_stack_pushed = false;

            std::vector<int64_t> catch_locations;
            std::vector<std::pair<int64_t, int64_t>> loop_locations;

            for (size_t pc = 0; pc < code.size(); ++pc)
            {
                try
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

                        call_stack_pushed = true;
                    }

                    switch (operation.id)
                    {
                        case OperationCode::Id::def_variable:
                            {
                                auto name = as_string(shared_from_this(), operation.value);
                                auto index = variables.insert({});
                                auto handler = [index](auto This) { This->push((int64_t)index); };

                                ADD_NATIVE_WORD(this,
                                                name,
                                                handler,
                                                "Access the variable " + name + ".",
                                                " -- variable_index");
                            }
                            break;

                        case OperationCode::Id::def_constant:
                            {
                                auto name = as_string(shared_from_this(), operation.value);
                                auto value = pop();

                                ADD_NATIVE_WORD(this,
                                                name,
                                                [value](auto This) { This->push(value); },
                                                "Constant value " + name + ".",
                                                " -- value");
                            }
                            break;

                        case OperationCode::Id::read_variable:
                            {
                                auto index = as_numeric<int64_t>(shared_from_this(), pop());
                                push(variables[index]);
                            }
                            break;

                        case OperationCode::Id::write_variable:
                            {
                                auto index = as_numeric<int64_t>(shared_from_this(), pop());
                                auto value = pop();

                                variables[index] = value;
                            }
                            break;

                        case OperationCode::Id::execute:
                            {
                                if (is_string(operation.value))
                                {
                                    auto name = as_string(shared_from_this(), operation.value);
                                    auto [found, word] = dictionary.find(name);

                                    if (!found)
                                    {
                                        throw_error(*this, "Word '" + name + "' not found.");
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
                                else if (is_numeric(operation.value))
                                {
                                    auto index = as_numeric<int64_t>(shared_from_this(),
                                                                    operation.value);
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
                                    throw_error(*this, "Can not execute unexpected value type.");
                                }
                            }
                            break;

                        case OperationCode::Id::word_index:
                            {
                                auto name = as_string(shared_from_this(), operation.value);
                                auto [ found, word ] = dictionary.find(name);

                                if (!found)
                                {
                                    throw_error(*this, "Word '" + name + "' not found.");
                                }

                                push((int64_t)word.handler_index);
                            }
                            break;

                        case OperationCode::Id::word_exists:
                            {
                                auto name = as_string(shared_from_this(), operation.value);
                                auto [ found, word ] = dictionary.find(name);

                                push(found);
                            }
                            break;

                        case OperationCode::Id::push_constant_value:
                            push(operation.value);
                            break;

                        case OperationCode::Id::mark_loop_exit:
                            {
                                int64_t relative_jump = as_numeric<int64_t>(shared_from_this(),
                                                                            operation.value);
                                int64_t absolute = pc + relative_jump;

                                loop_locations.push_back({pc + 1, absolute});
                            }
                            break;

                        case OperationCode::Id::unmark_loop_exit:
                            throw_error_if(loop_locations.empty(), *this,
                                           "Clearing a loop exit without an enclosing loop.");

                            loop_locations.pop_back();
                            break;

                        case OperationCode::Id::mark_catch:
                            {
                                int64_t relative_jump = as_numeric<int64_t>(shared_from_this(),
                                                                            operation.value);
                                int64_t absolute = pc + relative_jump;

                                catch_locations.push_back(absolute);
                            }
                            break;

                        case OperationCode::Id::unmark_catch:
                            throw_error_if(catch_locations.empty(), *this,
                                           "Clearing a catch exit without an enclosing try/catch.");
                            catch_locations.pop_back();
                            break;

                        case OperationCode::Id::jump:
                            pc += as_numeric<int64_t>(shared_from_this(), operation.value) - 1;
                            break;

                        case OperationCode::Id::jump_if_zero:
                            {
                                auto top = pop();
                                auto value = as_numeric<bool>(shared_from_this(), top);

                                if (!value)
                                {
                                    pc += as_numeric<int64_t>(shared_from_this(), operation.value) - 1;
                                }
                            }
                            break;

                        case OperationCode::Id::jump_if_not_zero:
                            {
                                auto top = pop();
                                auto value = as_numeric<bool>(shared_from_this(), top);

                                if (value)
                                {
                                    pc += as_numeric<int64_t>(shared_from_this(), operation.value) - 1;
                                }
                            }
                            break;

                        case OperationCode::Id::jump_loop_start:
                            if (!loop_locations.empty())
                            {
                                pc = loop_locations.back().first - 1;
                            }
                            break;

                        case OperationCode::Id::jump_loop_exit:
                            if (!loop_locations.empty())
                            {
                                pc = loop_locations.back().second - 1;
                                //loop_locations.pop_back();
                            }
                            break;

                        case OperationCode::Id::jump_target:
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

                    if (!catch_locations.empty())
                    {
                        pc = catch_locations.back() - 1;
                        catch_locations.pop_back();
                        push(std::string(error.what()));
                    }
                    else
                    {
                        throw error;
                    }
                }
            }

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
            for (const auto& value : stack)
            {
                if (std::holds_alternative<std::string>(value))
                {
                    stream << stringify(value) << std::endl;
                }
                else
                {
                    stream << value << std::endl;
                }
            }
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
                throw_error(*this, "Stack underflow.");
            }

            auto next = stack.front();
            stack.pop_front();

            return next;
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
                     Location(std::make_shared<std::filesystem::path>(path), line, column),
                     is_immediate,
                     false,
                     false,
                     description,
                     signature);
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


    InterpreterPtr create_interpreter()
    {
        return std::make_shared<InterpreterImpl>();
    }


}
