
#pragma once



namespace sorth
{


    class Interpreter;
    using InterpreterPtr = std::shared_ptr<Interpreter>;



    // The value type that represents a nothing value in the language.
    struct None
    {
    };


    class Array;
    using ArrayPtr = std::shared_ptr<Array>;

    class ByteBuffer;
    using ByteBufferPtr = std::shared_ptr<ByteBuffer>;

    class HashTable;
    using HashTablePtr = std::shared_ptr<HashTable>;

    class DataObject;
    using DataObjectPtr = std::shared_ptr<DataObject>;


    namespace internal
    {


        class Instruction;
        using ByteCode = std::vector<Instruction>;


        class Token;


    }


    // The value class represents all types an interpreter value can take in the language.
    class Value
    {
        private:
            using ValueType = std::variant<None,
                                           int64_t,
                                           double,
                                           bool,
                                           std::string,
                                           std::thread::id,
                                           DataObjectPtr,
                                           ArrayPtr,
                                           HashTablePtr,
                                           ByteBufferPtr,
                                           internal::Token,
                                           internal::ByteCode>;

        private:
            ValueType value;

        public:
            static thread_local size_t value_format_indent;

        public:
            Value() noexcept = default;
            Value(const None& none) noexcept;
            Value(int64_t value) noexcept;
            Value(size_t value) noexcept;
            Value(double value) noexcept;
            Value(bool value) noexcept;
            Value(const char* value) noexcept;
            Value(const std::string& value) noexcept;
            Value(const std::thread::id& value) noexcept;
            Value(const DataObjectPtr& value) noexcept;
            Value(const ArrayPtr& value) noexcept;
            Value(const HashTablePtr& value) noexcept;
            Value(const ByteBufferPtr& value) noexcept;
            Value(const internal::Token& value) noexcept;
            Value(const internal::ByteCode& value) noexcept;
            Value(const Value& value) = default;
            Value(Value&& value) = default;
            ~Value() noexcept = default;

        public:
            Value& operator =(const Value& value) = default;
            Value& operator =(Value&& value) = default;

            Value& operator =(const None& none) noexcept;
            Value& operator =(int64_t value) noexcept;
            Value& operator =(double value) noexcept;
            Value& operator =(bool value) noexcept;
            Value& operator =(const char* value) noexcept;
            Value& operator =(const std::string& value) noexcept;
            Value& operator =(const std::thread::id& value) noexcept;
            Value& operator =(const DataObjectPtr& value) noexcept;
            Value& operator =(const ArrayPtr& value) noexcept;
            Value& operator =(const HashTablePtr& value) noexcept;
            Value& operator =(const ByteBufferPtr& value) noexcept;
            Value& operator =(const internal::Token& value) noexcept;
            Value& operator =(const internal::ByteCode& value) noexcept;

            operator bool() const noexcept;

        public:
            inline size_t type_index() const noexcept
            {
                return value.index();
            }

        public:
            Value deep_copy() const;

        public:
            bool is_none() const noexcept;
            bool is_numeric() const noexcept;
            bool is_integer() const noexcept;
            bool is_float() const noexcept;
            bool is_bool() const noexcept;
            bool is_string() const noexcept;
            bool is_thread_id() const noexcept;
            bool is_structure() const noexcept;
            bool is_array() const noexcept;
            bool is_hash_table() const noexcept;
            bool is_byte_buffer() const noexcept;
            bool is_token() const noexcept;
            bool is_byte_code() const noexcept;

        public:
            static bool either_is_string(const Value& a, const Value& b) noexcept;

            static bool either_is_numeric(const Value& a, const Value& b) noexcept;

            static bool either_is_integer(const Value& a, const Value& b) noexcept;
            static bool either_is_float(const Value& a, const Value& b) noexcept;

        public:
            int64_t as_integer(const InterpreterPtr& interpreter) const;
            double as_float(const InterpreterPtr& interpreter) const;
            bool as_bool() const noexcept;
            std::string as_string(const InterpreterPtr& interpreter) const;
            std::thread::id as_thread_id(const InterpreterPtr& interpreter) const;
            std::string as_string_with_conversion() const noexcept;
            DataObjectPtr as_structure(const InterpreterPtr& interpreter) const;
            ArrayPtr as_array(const InterpreterPtr& interpreter) const;
            HashTablePtr as_hash_table(const InterpreterPtr& interpreter) const;
            ByteBufferPtr as_byte_buffer(const InterpreterPtr& interpreter) const;
            internal::Token as_token(const InterpreterPtr& interpreter) const;
            internal::ByteCode as_byte_code(const InterpreterPtr& interpreter) const;

        public:
            size_t hash() const noexcept;
            static size_t hash_combine(size_t seed, size_t value) noexcept;

        private:
            friend std::ostream& operator <<(std::ostream& stream, const Value& value) noexcept;
            friend std::strong_ordering operator <=>(const Value& rhs, const Value& lhs) noexcept;
    };




    // Return a string value, but convert the value to a string if it is not a string.  Also enclose
    // the string in quotes, and will escape any characters that need to be escaped.
    std::string stringify(const Value& value) noexcept;

    // Enclose a string in quotes and escape any characters that need to be escaped.
    std::string stringify(const std::string& value) noexcept;


    std::ostream& operator <<(std::ostream& stream, const Value& value) noexcept;


    std::strong_ordering operator <=>(const Value& rhs, const Value& lhs) noexcept;


    inline bool operator ==(const Value& rhs, const Value& lhs) noexcept
    {
        return (rhs <=> lhs) == std::strong_ordering::equal;
    }

    inline bool operator !=(const Value& rhs, const Value& lhs) noexcept
    {
        return (rhs <=> lhs) != std::strong_ordering::equal;
    }


}


namespace std
{


    template<>
    struct hash<sorth::Value>
    {
        inline size_t operator()(const sorth::Value& key) const
        {
            return key.hash();
        }
    };


}
