
#include "sorth.h"
#include "sorth_ext.h"

#ifdef IS_WINDOWS

    #include <windows.h>

#elif defined(IS_UNIX)

    #include <dlfcn.h>

#endif



struct Value_t
{
    sorth::Value value;
};


struct Interpreter_t
{
    sorth::InterpreterPtr interpreter;
};



namespace sorth
{


    namespace
    {


        SorthApi_t new_api_ref();



        ValueRef_t new_value()
        {
            return new Value_t { .value = sorth::Value() };
        }


        void free_value(ValueRef_t value_ref)
        {
            delete value_ref;
        }


        int64_t as_int(InterpreterRef_t interpreter_ref, ValueRef_t value_ref)
        {
            if (is_numeric(value_ref->value))
            {
                return as_numeric<int64_t>(interpreter_ref->interpreter,
                                           value_ref->value);
            }

            return 0;
        }


        double as_float(InterpreterRef_t interpreter_ref, ValueRef_t value_ref)
        {
            if (is_numeric(value_ref->value))
            {
                return as_numeric<double>(interpreter_ref->interpreter,
                                          value_ref->value);
            }

            return 0;
        }


        bool as_bool(InterpreterRef_t interpreter_ref, ValueRef_t value_ref)
        {
            if (is_numeric(value_ref->value))
            {
                return as_numeric<bool>(interpreter_ref->interpreter,
                                        value_ref->value);
            }

            return false;
        }


        char* as_string(InterpreterRef_t interpreter_ref, ValueRef_t value_ref)
        {
            if (is_string(value_ref->value))
            {
                std::string string_val = as_string(interpreter_ref->interpreter,
                                                   value_ref->value);
                char* str_value = new char[string_val.size() + 1] { 0 };

                std::memcpy(str_value, string_val.c_str(), string_val.length());

                return str_value;
            }

            return new char[1] { 0 };
        }


        void free_string(char* str)
        {
            delete [] str;
        }


        bool is_numeric(ValueRef_t value_ref)
        {
            return is_numeric(value_ref->value);
        }


        bool is_string(ValueRef_t value_ref)
        {
            return is_string(value_ref->value);
        }


        void set_int(ValueRef_t value_ref, int64_t new_value)
        {
            value_ref->value = new_value;
        }


        void set_float(ValueRef_t value_ref, double new_value)
        {
            value_ref->value = new_value;
        }


        void set_bool(ValueRef_t value_ref, bool new_value)
        {
            value_ref->value = new_value;
        }


        void set_string(ValueRef_t value_ref, const char* new_value)
        {
            value_ref->value = new_value;
        }


        void halt(InterpreterRef_t interpreter_ref)
        {
            interpreter_ref->interpreter->halt();
        }


        void clear_halt_flag(InterpreterRef_t interpreter_ref)
        {
            interpreter_ref->interpreter->clear_halt_flag();
        }


        void push(InterpreterRef_t interpreter_ref, ValueRef_t value_ref)
        {
            interpreter_ref->interpreter->push(value_ref->value);
        }


        ValueRef_t pop(InterpreterRef_t interpreter_ref)
        {
            ValueRef_t value_ref = nullptr;

            try
            {
                value_ref = new_value();
                value_ref->value = interpreter_ref->interpreter->pop();
            }
            catch (...)
            {
            }

            return value_ref;
        }


        void add_word(InterpreterRef_t interpreter_ref,
                            const char* name,
                            WordHandlerRef_t handler,
                            const char* file,
                            size_t line,
                            bool is_immediate,
                            const char* description,
                            const char* signature)
        {
            auto wrapped_handler = [name, handler](InterpreterPtr& interpreter)
                {
                    auto interpreter_ref = Interpreter_t { .interpreter = interpreter };
                    static auto api_ref = new_api_ref();

                    try
                    {
                        auto result = handler(&interpreter_ref, &api_ref);

                        if (!result.was_successful)
                        {
                            if (result.error_message != nullptr)
                            {
                                internal::throw_error(*interpreter, result.error_message);
                            }
                            else
                            {
                                internal::throw_error(*interpreter,
                                         std::string("The word, ") + name + ", returned an error.");
                            }
                        }
                    }
                    catch (...)
                    {
                        internal::throw_error("External exception thrown from extension "
                                              "word, " + std::string(name) + ".");
                    }
                };

            interpreter_ref->interpreter->add_word(name,
                                                wrapped_handler,
                                                file,
                                                line,
                                                1,
                                                is_immediate,
                                                description,
                                                signature);
        }


        static SorthApi_t new_api_ref()
        {
            return SorthApi_t {
                    .new_value = new_value,
                    .free_value = free_value,
                    .as_int = as_int,
                    .as_float = as_float,
                    .as_bool = as_bool,
                    .as_string = as_string,
                    .free_string = free_string,
                    .is_numeric = is_numeric,
                    .is_string = is_string,
                    .set_int = set_int,
                    .set_float = set_float,
                    .set_bool = set_bool,
                    .set_string = set_string,
                    .halt = halt,
                    .clear_halt_flag = clear_halt_flag,
                    .push = push,
                    .pop = pop,
                    .add_word = add_word
                };
        }



        void word_module(InterpreterPtr& interpreter)
        {
            auto& current_token = interpreter->constructor()->current_token;
            ++current_token;

            auto& token = interpreter->constructor()->input_tokens[current_token];
            auto module_name = token.text;

            HandlerRegistrationRef_t registration = nullptr;

            #ifdef IS_WINDOWS

                module_name += ".dll";

                HMODULE handle = LoadLibraryA(module_name.c_str());

                throw_error_if(handle == nullptr,
                               token.location,
                               "Failed to load external library.");

                registration =
                          (HandlerRegistrationRef_t)GetProcAddress(handle, "register_module_words");

                throw_error_if(registration == nullptr,
                               token.location,
                               "Could not find the registration function in the library, " +
                               module_name + ".");

            #elif defined(IS_UNIX)

                #ifdef IS_MACOS
                    module_name += ".dylib";
                #else
                    module_name += ".so";
                #endif

                void* handle = dlopen(module_name.c_str(), RTLD_NOW);

                throw_error_if(handle == nullptr,
                               token.location,
                               std::string("Failed to load external library:") + dlerror() + ".");

                HandlerRegistrationRef_t registration =
                                   (HandlerRegistrationRef_t)dlsym(handle, "register_module_words");

                throw_error_if(registration == nullptr,
                               token.location,
                               "Could not find the registration function in the library, " +
                               module_name + ": " + dlerror() + ".");

            #endif

            auto interpreter_ref = Interpreter_t { .interpreter = interpreter };
            auto api_ref = new_api_ref();

            registration(&interpreter_ref, &api_ref);
        }



    }



    void register_extension_module_words(InterpreterPtr& interpreter)
    {
        ADD_IMMEDIATE_WORD(interpreter, "module", word_module,
                           "Load a library of native words.",
                           "module \"module_name\"");
    }


}
