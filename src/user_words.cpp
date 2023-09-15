
#include "sorth.h"



namespace sorth
{


    using namespace internal;


    namespace
    {


        void word_user_env_read(InterpreterPtr& interpreter)
        {
            auto name = as_string(interpreter, interpreter->pop());
            const char* value = std::getenv(name.c_str());

            if (value != nullptr)
            {
                interpreter->push(value);
            }
            else
            {
                interpreter->push("");
            }
        }


        void word_user_os_read(InterpreterPtr& interpreter)
        {
            #ifdef __APPLE__
                interpreter->push("MacOS");
            #else
                interpreter->push("unknown");
            #endif
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
    }


}
