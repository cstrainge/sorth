
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


}
