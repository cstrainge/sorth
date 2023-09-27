
#include "sorth.h"


namespace sorth::internal
{


    [[noreturn]]
    void throw_error(const std::string& message)
    {
        throw std::runtime_error(message);
    }


    [[noreturn]]
    void throw_error(const Location& location, const std::string& message)
    {
        std::stringstream stream;

        stream << location << ": Error: " << message;
        throw_error(stream.str());
    }


    void throw_error_if(bool condition, const Location& location, const std::string& message)
    {
        if (condition)
        {
            throw_error(location, message);
        }
    }


    [[noreturn]]
    void throw_error(const Interpreter& interpreter, const std::string& message)
    {
        auto current_location = interpreter.get_current_location();
        auto call_stack = interpreter.get_call_stack();

        std::stringstream stream;

        stream << current_location << ": Error: " << message;

        if (!call_stack.empty())
        {
            stream << std::endl
                   << std::endl
                   << "Call stack:" << std::endl
                   << call_stack;
        }

        throw_error(stream.str());
    }


    void throw_error_if(bool condition, const Interpreter& interpreter,
                        const std::string& message)
    {
        if (condition)
        {
            throw_error(interpreter, message);
        }
    }



    #if defined(_WIN64)


        [[noreturn]]
        void throw_windows_error(const Interpreter& interpreter,
                                 const std::string& message,
                                 DWORD code)
        {
            char message_buffer[4096 + 1];
            memset(message_buffer, 0, 4096 + 1);
            size_t size = 4096;

            size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                                  FORMAT_MESSAGE_IGNORE_INSERTS,
                                  nullptr,
                                  code,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  message_buffer,
                                  size,
                                  nullptr);

            std::stringstream stream;

            stream << message << message_buffer;

            throw_error(interpreter, stream.str());
        }



        void throw_windows_error_if(bool condition, const Interpreter& interpreter,
                                    const std::string& message,
                                    DWORD code)
        {
            if (condition)
            {
                throw_windows_error(interpreter, message, code);
            }
        }



    #endif



}
