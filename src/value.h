
#pragma once


namespace sorth
{


    namespace internal
    {
        struct OperationCode;
        using ByteCode = std::vector<OperationCode>;
    }

    class Interpreter;
    using InterpreterPtr = std::shared_ptr<Interpreter>;

    struct DataObject;
    using DataObjectPtr = std::shared_ptr<DataObject>;

    class Array;
    using ArrayPtr = std::shared_ptr<Array>;

    class ByteBuffer;
    using ByteBufferPtr = std::shared_ptr<ByteBuffer>;

    class HashTable;
    using HashTablePtr = std::shared_ptr<HashTable>;


    using Value = std::variant<int64_t,
                               double,
                               bool,
                               std::string,
                               DataObjectPtr,
                               ArrayPtr,
                               ByteBufferPtr,
                               HashTablePtr,
                               internal::Token,
                               internal::Location,
                               internal::ByteCode>;

    using ValueStack = std::list<Value>;
    using ValueList = std::vector<Value>;

    using VariableList = internal::ContextualList<Value>;

    std::ostream& operator <<(std::ostream& stream, const Value& value);


    size_t hash_value(const Value& key);
    bool operator ==(const sorth::Value& rhs, const sorth::Value& lhs);


}


inline std::string stringify(const std::string& input)
{
    std::string output = "\"";

    for (size_t i = 0; i < input.size(); ++i)
    {
        char next = input[i];

        switch (next)
        {
            case '\r': output += "\\r";  break;
            case '\n': output += "\\n";  break;
            case '\t': output += "\\n";  break;
            case '\"': output += "\\\""; break;

            default:   output += next;   break;
        }
    }

    output += "\"";

    return output;
}


inline std::string stringify(const sorth::Value& value)
{
    std::string result;

    if (std::holds_alternative<std::string>(value))
    {
        result = stringify(std::get<std::string>(value));
    }
    else
    {
        std::stringstream stream;

        stream << value;
        result = stream.str();
    }

    return result;
}


namespace std
{


    template<>
    struct hash<sorth::Value>
    {
        size_t operator()(const sorth::Value& key) const
        {
            return sorth::hash_value(key);
        }
    };


}
