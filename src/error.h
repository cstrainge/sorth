
#pragma once


namespace sorth
{


    class Interpreter;


}


namespace sorth::internal
{


    [[noreturn]]
    void throw_error(const std::string& message);

    [[noreturn]]
    void throw_error(const Location& location, const std::string& message);

    void throw_error_if(bool condition, const Location& location, const std::string& message);


    [[noreturn]]
    void throw_error(const Interpreter& interpreter, const std::string& message);

    void throw_error_if(bool condition, const Interpreter& interpreter,
                        const std::string& message);



    #if defined(_WIN64)


        [[noreturn]]
        void throw_windows_error(const Interpreter& interpreter,
                                 const std::string& message,
                                 DWORD code);

        void throw_windows_error_if(bool condition,
                                    const Interpreter& interpreter,
                                    const std::string& message,
                                    DWORD code);


    #endif


}
