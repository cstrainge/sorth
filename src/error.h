
#pragma once


namespace sorth::internal
{


    [[noreturn]]
    void throw_error(const std::string& message);

    [[noreturn]]
    void throw_error(const Location& location, const std::string& message);

    void throw_error_if(bool condition, const Location& location, const std::string& message);


    std::ostream& operator <<(std::ostream& stream, const std::runtime_error& error);


}
