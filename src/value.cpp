
#include "sorth.h"



namespace sorth
{


    // Let's make sure we can convert values to text for displaying to the user among various other
    // uses like writing to a text file.
    template <typename variant>
    inline void value_print_if(std::ostream& stream, const Value& variant_value)
    {
        if (const variant* value = std::get_if<variant>(&variant_value))
        {
            stream << *value;
        }
    }


    template <>
    inline void value_print_if<bool>(std::ostream& stream, const Value& variant_value)
    {
        if (const bool* value = std::get_if<bool>(&variant_value))
        {
            stream << std::boolalpha << *value;
        }
    }


    std::ostream& operator <<(std::ostream& stream, const Value& value)
    {
        value_print_if<int64_t>(stream, value);
        value_print_if<double>(stream, value);
        value_print_if<bool>(stream, value);
        value_print_if<std::string>(stream, value);
        value_print_if<DataObjectPtr>(stream, value);
        value_print_if<ArrayPtr>(stream, value);
        value_print_if<ByteBufferPtr>(stream, value);
        value_print_if<std::runtime_error>(stream, value);
        value_print_if<internal::Token>(stream, value);
        value_print_if<internal::Location>(stream, value);
        value_print_if<internal::ByteCode>(stream, value);

        return stream;
    }


}
