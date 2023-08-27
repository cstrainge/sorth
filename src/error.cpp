
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


    std::ostream& operator <<(std::ostream& stream, const std::runtime_error& error)
    {
        stream << std::endl
               << error.what() << std::endl;

        /*if (!call_stack.empty())
        {
            stream << std::endl
                   << "Call stack:" << std::endl
                   << call_stack << std::endl;
        }*/

        return stream;
    }


}
