
#pragma once


namespace sorth
{


    class Interpreter;


}


namespace sorth::internal
{


    /*class ScriptError : public std::runtime_error
    {
        public:
            ScriptError(const std::string& message);
    };*/


    [[noreturn]]
    void throw_error(const std::string& message);

    [[noreturn]]
    void throw_error(const Location& location, const std::string& message);

    void throw_error_if(bool condition, const Location& location, const std::string& message);


    [[noreturn]]
    void throw_error(const Interpreter& interpreter, const std::string& message);

    void throw_error_if(bool condition, const Interpreter& interpreter,
                        const std::string& message);


}
