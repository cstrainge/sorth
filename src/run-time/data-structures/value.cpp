
#include "sorth.h"



namespace sorth
{


    using namespace internal;


    std::string stringify(const Value& value) noexcept
    {
        return stringify(value.as_string_with_conversion());
    }


    std::string stringify(const std::string& value) noexcept
    {
        std::string output = "\"";

        for (size_t i = 0; i < value.size(); ++i)
        {
            char next = value[i];

            switch (next)
            {
                case '\r': output += "\\r";  break;
                case '\n': output += "\\n";  break;
                case '\t': output += "\\n";  break;
                case '\"': output += "\\\""; break;

                default:
                    if (!isprint(next))
                    {
                        output += "\\0" + std::to_string((int)next);
                    }
                    else
                    {
                        output += next;
                    }
                    break;
            }
        }

        output += "\"";

        return output;
    }


    std::ostream& operator <<(std::ostream& stream, const Value& value) noexcept
    {
        if (std::holds_alternative<None>(value.value))
        {
            stream << "none";
        }
        else if (std::holds_alternative<int64_t>(value.value))
        {
            stream << std::get<int64_t>(value.value);
        }
        else if (std::holds_alternative<double>(value.value))
        {
            stream << std::get<double>(value.value);
        }
        else if (std::holds_alternative<bool>(value.value))
        {
            stream << (std::get<bool>(value.value) ? "true" : "false");
        }
        else if (std::holds_alternative<std::string>(value.value))
        {
            stream << std::get<std::string>(value.value);
        }
        else if (std::holds_alternative<DataObjectPtr>(value.value))
        {
            stream << std::get<DataObjectPtr>(value.value);
        }
        else if (std::holds_alternative<ArrayPtr>(value.value))
        {
            stream << std::get<ArrayPtr>(value.value);
        }
        else if (std::holds_alternative<HashTablePtr>(value.value))
        {
            stream << std::get<HashTablePtr>(value.value);
        }
        else if (std::holds_alternative<ByteBufferPtr>(value.value))
        {
            stream << std::get<ByteBufferPtr>(value.value);
        }
        else if (std::holds_alternative<internal::Token>(value.value))
        {
            stream << std::get<internal::Token>(value.value);
        }
        else if (std::holds_alternative<internal::ByteCode>(value.value))
        {
            stream << std::get<internal::ByteCode>(value.value);
        }
        else
        {
            stream << "<unknown-value-type>";
        }

        return stream;
    }


    std::strong_ordering operator <=>(const Value& rhs, const Value& lhs) noexcept
    {
        if (rhs.value.index() != lhs.value.index())
        {
            return rhs.value.index() <=> lhs.value.index();
        }

        if (std::holds_alternative<None>(rhs.value))
        {
            return std::strong_ordering::equal;
        }

        if (std::holds_alternative<int64_t>(rhs.value))
        {
            return std::get<int64_t>(rhs.value) <=> std::get<int64_t>(lhs.value);
        }

        if (std::holds_alternative<double>(rhs.value))
        {
            auto lhs_value = std::get<double>(lhs.value);
            auto rhs_value = std::get<double>(rhs.value);

            if (lhs_value > rhs_value)
            {
                return std::strong_ordering::greater;
            }
            else if (lhs_value < rhs_value)
            {
                return std::strong_ordering::less;
            }

            return std::strong_ordering::equal;
        }

        if (std::holds_alternative<bool>(rhs.value))
        {
            return std::get<bool>(rhs.value) <=> std::get<bool>(lhs.value);
        }

        if (std::holds_alternative<std::string>(rhs.value))
        {
            return std::get<std::string>(rhs.value) <=> std::get<std::string>(lhs.value);
        }

        if (std::holds_alternative<DataObjectPtr>(rhs.value))
        {
            return std::get<DataObjectPtr>(rhs.value) <=> std::get<DataObjectPtr>(lhs.value);
        }

        if (std::holds_alternative<ArrayPtr>(rhs.value))
        {
            return std::get<ArrayPtr>(rhs.value) <=> std::get<ArrayPtr>(lhs.value);
        }

        if (std::holds_alternative<HashTablePtr>(rhs.value))
        {
            return std::get<HashTablePtr>(rhs.value) <=> std::get<HashTablePtr>(lhs.value);
        }

        return std::get<ByteBufferPtr>(rhs.value) <=> std::get<ByteBufferPtr>(lhs.value);
    }


    thread_local size_t Value::value_format_indent;


    Value::Value(const None& none) noexcept
    : value(none)
    {
    }


    Value::Value(int64_t value) noexcept
    : value(value)
    {
    }


    Value::Value(size_t value) noexcept
    : value(static_cast<int64_t>(value))
    {
    }


    Value::Value(double value) noexcept
    : value(value)
    {
    }


    Value::Value(bool value) noexcept
    : value(value)
    {
    }


    Value::Value(const char* value) noexcept
    : value(std::string(value))
    {
    }


    Value::Value(const std::string& value) noexcept
    : value(value)
    {
    }


    Value::Value(const std::thread::id& value) noexcept
    : value(value)
    {
    }


    Value::Value(const DataObjectPtr& value) noexcept
    : value(value)
    {
    }


    Value::Value(const ArrayPtr& value) noexcept
    : value(value)
    {
    }


    Value::Value(const HashTablePtr& value) noexcept
    : value(value)
    {
    }


    Value::Value(const ByteBufferPtr& value) noexcept
    : value(value)
    {
    }


    Value::Value(const internal::Token& value) noexcept
    : value(value)
    {
    }


    Value::Value(const internal::ByteCode& value) noexcept
    : value(value)
    {
    }


    Value& Value::operator =(const None& none) noexcept
    {
        value = none;
        return *this;
    }


    Value& Value::operator =(int64_t new_value) noexcept
    {
        value = new_value;
        return *this;
    }


    Value& Value::operator =(double new_value) noexcept
    {
        value = new_value;
        return *this;
    }


    Value& Value::operator =(bool new_value) noexcept
    {
        value = new_value;

        return *this;
    }


    Value& Value::operator =(const char* new_value) noexcept
    {
        value = std::string(new_value);

        return *this;
    }


    Value& Value::operator =(const std::string& new_value) noexcept
    {
        value = new_value;

        return *this;
    }


    Value& Value::operator =(const std::thread::id& new_value) noexcept
    {
        value = new_value;

        return *this;
    }


    Value& Value::operator =(const DataObjectPtr& new_value) noexcept
    {
        value = new_value;
        return *this;
    }


    Value& Value::operator =(const ArrayPtr& new_value) noexcept
    {
        value = new_value;
        return *this;
    }


    Value& Value::operator =(const HashTablePtr& new_value) noexcept
    {
        value = new_value;
        return *this;
    }


    Value& Value::operator =(const ByteBufferPtr& new_value) noexcept
    {
        value = new_value;
        return *this;
    }


    Value& Value::operator =(const internal::Token& new_value) noexcept
    {
        value = new_value;
        return *this;
    }


    Value& Value::operator =(const internal::ByteCode& new_value) noexcept
    {
        value = new_value;
        return *this;
    }


    Value::operator bool() const noexcept
    {
        return as_bool();
    }


    Value Value::deep_copy() const
    {
        if (std::holds_alternative<ArrayPtr>(value))
        {
            auto array = std::get<ArrayPtr>(value);
            auto new_array = std::make_shared<Array>(*array);

            return new_array;
        }

        if (std::holds_alternative<DataObjectPtr>(value))
        {
            auto structure = std::get<DataObjectPtr>(value);
            auto new_structure = make_data_object(structure->definition);

            for (size_t i = 0; i < structure->fields.size(); ++i)
            {
                new_structure->fields[i] = structure->fields[i].deep_copy();
            }

            return new_structure;
        }

        if (std::holds_alternative<HashTablePtr>(value))
        {
            auto hash_table = std::get<HashTablePtr>(value);
            auto new_hash_table = std::make_shared<HashTable>();

            for (auto& [key, value] : hash_table->get_items())
            {
                new_hash_table->insert(key.deep_copy(), value.deep_copy());
            }

            return new_hash_table;
        }

        if (std::holds_alternative<ByteBufferPtr>(value))
        {
            auto byte_buffer = std::get<ByteBufferPtr>(value);
            auto new_byte_buffer = std::make_shared<ByteBuffer>(*byte_buffer);

            return byte_buffer;
        }

        return Value(*this);
    }


    bool Value::is_none() const noexcept
    {
        return std::holds_alternative<None>(value);
    }


    bool Value::is_string() const noexcept
    {
        if (std::holds_alternative<std::string>(value))
        {
            return true;
        }

        if (std::holds_alternative<Token>(value))
        {
            auto token = std::get<Token>(value);

            return true; //token.type == Token::Type::string || token.type == Token::Type::word;
        }

        return false;
    }


    bool Value::is_thread_id() const noexcept
    {
        return std::holds_alternative<std::thread::id>(value);
    }


    bool Value::is_numeric() const noexcept
    {
        return    std::holds_alternative<int64_t>(value)
               || std::holds_alternative<double>(value)
               || std::holds_alternative<bool>(value);
    }


    bool Value::is_integer() const noexcept
    {
        return std::holds_alternative<int64_t>(value);
    }


    bool Value::is_float() const noexcept
    {
        return std::holds_alternative<double>(value);
    }


    bool Value::is_bool() const noexcept
    {
        return std::holds_alternative<bool>(value);
    }


    bool Value::is_structure() const noexcept
    {
        return std::holds_alternative<DataObjectPtr>(value);
    }


    bool Value::is_array() const noexcept
    {
        return std::holds_alternative<ArrayPtr>(value);
    }


    bool Value::is_hash_table() const noexcept
    {
        return std::holds_alternative<HashTablePtr>(value);
    }


    bool Value::is_byte_buffer() const noexcept
    {
        return std::holds_alternative<ByteBufferPtr>(value);
    }


    bool Value::is_token() const noexcept
    {
        return std::holds_alternative<Token>(value);
    }


    bool Value::is_byte_code() const noexcept
    {
        return std::holds_alternative<ByteCode>(value);
    }


    bool Value::either_is_string(const Value& a, const Value& b) noexcept
    {
        return a.is_string() || b.is_string();
    }


    bool Value::either_is_numeric(const Value& a, const Value& b) noexcept
    {
        return a.is_numeric() || b.is_numeric();
    }


    bool Value::either_is_integer(const Value& a, const Value& b) noexcept
    {
        return std::holds_alternative<int64_t>(a.value) || std::holds_alternative<int64_t>(b.value);
    }


    bool Value::either_is_float(const Value& a, const Value& b) noexcept
    {
        return std::holds_alternative<double>(a.value) || std::holds_alternative<double>(b.value);
    }


    int64_t Value::as_integer(const InterpreterPtr& interpreter) const
    {
        if (std::holds_alternative<None>(value))
        {
            return 0;
        }

        if (std::holds_alternative<int64_t>(value))
        {
            return std::get<int64_t>(value);
        }

        if (std::holds_alternative<double>(value))
        {
            return static_cast<int64_t>(std::get<double>(value));
        }

        if (std::holds_alternative<bool>(value))
        {
            return std::get<bool>(value) ? 1 : 0;
        }

        throw_error(interpreter, "Expected numeric or boolean value.");
    }


    double Value::as_float(const InterpreterPtr& interpreter) const
    {
        if (std::holds_alternative<None>(value))
        {
            return 0.0;
        }

        if (std::holds_alternative<int64_t>(value))
        {
            return static_cast<double>(std::get<int64_t>(value));
        }

        if (std::holds_alternative<double>(value))
        {
            return std::get<double>(value);
        }

        if (std::holds_alternative<bool>(value))
        {
            return std::get<bool>(value) ? 1.0 : 0.0;
        }

        throw_error(interpreter, "Expected numeric or boolean value.");
    }


    bool Value::as_bool() const noexcept
    {
        if (std::holds_alternative<None>(value))
        {
            return false;
        }

        if (std::holds_alternative<int64_t>(value))
        {
            return std::get<int64_t>(value) != 0;
        }

        if (std::holds_alternative<double>(value))
        {
            return std::get<double>(value) != 0.0;
        }

        if (std::holds_alternative<bool>(value))
        {
            return std::get<bool>(value);
        }

        if (std::holds_alternative<std::string>(value))
        {
            return !std::get<std::string>(value).empty();
        }

        return true;
    }


    std::string Value::as_string(const InterpreterPtr& interpreter) const
    {
        if (std::holds_alternative<std::string>(value))
        {
            return std::get<std::string>(value);
        }

        if (std::holds_alternative<Token>(value))
        {
            auto token = std::get<Token>(value);

            //if (   (token.type == Token::Type::string)
            //    || (token.type == Token::Type::word))
            //{
                return token.text;
            //}
        }

        throw_error(interpreter, "Expected string value.");
    }


    std::thread::id Value::as_thread_id(const InterpreterPtr& interpreter) const
    {
        if (!std::holds_alternative<std::thread::id>(value))
        {
            throw_error(interpreter, "Expected thread ID value.");
        }

        return std::get<std::thread::id>(value);
    }


    std::string Value::as_string_with_conversion() const noexcept
    {
        if (std::holds_alternative<std::string>(value))
        {
            return std::get<std::string>(value);
        }
        else if (std::holds_alternative<Token>(value))
        {
            return std::get<Token>(value).text;
        }

        std::stringstream stream;

        stream << *this;

        return stream.str();
    }


    DataObjectPtr Value::as_structure(const InterpreterPtr& interpreter) const
    {
        if (!std::holds_alternative<DataObjectPtr>(value))
        {
            throw_error(interpreter, "Expected structure value.");
        }

        return std::get<DataObjectPtr>(value);
    }


    ArrayPtr Value::as_array(const InterpreterPtr& interpreter) const
    {
        if (!std::holds_alternative<ArrayPtr>(value))
        {
            throw_error(interpreter, "Expected array value.");
        }

        return std::get<ArrayPtr>(value);
    }


    HashTablePtr Value::as_hash_table(const InterpreterPtr& interpreter) const
    {
        if (!std::holds_alternative<HashTablePtr>(value))
        {
            throw_error(interpreter, "Expected hash table value.");
        }

        return std::get<HashTablePtr>(value);
    }


    ByteBufferPtr Value::as_byte_buffer(const InterpreterPtr& interpreter) const
    {
        if (!std::holds_alternative<ByteBufferPtr>(value))
        {
            throw_error(interpreter, "Expected byte buffer value.");
        }

        return std::get<ByteBufferPtr>(value);
    }


    Token Value::as_token(const InterpreterPtr& interpreter) const
    {
        if (!std::holds_alternative<Token>(value))
        {
            throw_error(interpreter, "Expected token value.");
        }

        return std::get<Token>(value);
    }


    ByteCode Value::as_byte_code(const InterpreterPtr& interpreter) const
    {
        if (!std::holds_alternative<ByteCode>(value))
        {
            throw_error(interpreter, "Expected byte code value.");
        }

        return std::get<ByteCode>(value);
    }


    size_t Value::hash() const noexcept
    {
        if (std::holds_alternative<None>(value))
        {
            return std::hash<int>()(0);
        }

        if (std::holds_alternative<int64_t>(value))
        {
            return std::hash<int64_t>()(std::get<int64_t>(value));
        }

        if (std::holds_alternative<double>(value))
        {
            return std::hash<double>()(std::get<double>(value));
        }

        if (std::holds_alternative<bool>(value))
        {
            return std::hash<bool>()(std::get<bool>(value));
        }

        if (std::holds_alternative<std::string>(value))
        {
            return std::hash<std::string>()(std::get<std::string>(value));
        }

        if (std::holds_alternative<DataObjectPtr>(value))
        {
            return std::get<DataObjectPtr>(value)->hash();
        }

        if (std::holds_alternative<ArrayPtr>(value))
        {
            return std::get<ArrayPtr>(value)->hash();
        }

        if (std::holds_alternative<HashTablePtr>(value))
        {
            return std::get<HashTablePtr>(value)->hash();
        }

        if (std::holds_alternative<ByteBufferPtr>(value))
        {
            return std::get<ByteBufferPtr>(value)->hash();
        }

        return 0;
    }


    size_t Value::hash_combine(size_t seed, size_t value) noexcept
    {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }


}
