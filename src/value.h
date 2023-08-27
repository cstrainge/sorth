
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


    using Value = std::variant<int64_t,
                               double,
                               bool,
                               std::string,
                               DataObjectPtr,
                               ArrayPtr,
                               ByteBufferPtr,
                               internal::Token,
                               internal::Location,
                               internal::ByteCode>;

    using ValueStack = std::list<Value>;
    using ValueList = std::vector<Value>;

    using VariableList = internal::ContextualList<Value>;

    std::ostream& operator <<(std::ostream& stream, const Value& value);


}
