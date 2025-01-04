
#include "sorth.h"



namespace sorth
{


    using namespace internal;


    namespace
    {


        void word_user_env_read(InterpreterPtr& interpreter)
        {
            auto name = interpreter->pop_as_string();
            const char* value = std::getenv(name.c_str());

            if (value != nullptr)
            {
                interpreter->push(std::string(value));
            }
            else
            {
                interpreter->push(std::string(""));
            }
        }


        void word_user_os_read(InterpreterPtr& interpreter)
        {
            #ifdef __APPLE__

                interpreter->push(std::string("macOS"));

            #elif __linux__

                interpreter->push(std::string("Linux"));

            #elif _WIN32 || _WIN64

                interpreter->push(std::string("Windows"));

            #else

                interpreter->push(std::string("Unknown"));

            #endif
        }


        void word_user_cwd(InterpreterPtr& interpreter)
        {
            auto cwd = std::filesystem::current_path();

            interpreter->push(cwd.string());
        }


    }


    void register_user_words(InterpreterPtr& interpreter)
    {
        // User environment words.
        ADD_NATIVE_WORD(interpreter, "user.env@", word_user_env_read,
                        "Read an environment variable",
                        "name -- value_or_empty");

        ADD_NATIVE_WORD(interpreter, "user.os", word_user_os_read,
                        "Get the name of the OS the script is running under.",
                        " -- os_name");

        ADD_NATIVE_WORD(interpreter, "user.cwd", word_user_cwd,
            "Get the process's current working directory.",
            " -- directory_path");
    }


}
