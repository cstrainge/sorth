
#include "sorth.h"



namespace sorth
{


    using namespace internal;


    thread_local uint64_t value_print_indent = 0;


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
        value_print_if<HashTablePtr>(stream, value);
        value_print_if<ByteBufferPtr>(stream, value);
        value_print_if<Token>(stream, value);
        value_print_if<Location>(stream, value);
        value_print_if<ByteCode>(stream, value);
        value_print_if<std::thread::id>(stream, value);

        return stream;
    }


    size_t hash_value(const Value& key)
    {
        if (std::holds_alternative<int64_t>(key))
        {
            return std::hash<int64_t>()(std::get<int64_t>(key));
        }

        if (std::holds_alternative<double>(key))
        {
            return std::hash<double>()(std::get<double>(key));
        }

        if (std::holds_alternative<std::string>(key))
        {
            return std::hash<std::string>()(std::get<std::string>(key));
        }

        if (std::holds_alternative<DataObjectPtr>(key))
        {
            auto value = std::get<DataObjectPtr>(key);
            size_t result = 0;

            for (auto field : value->fields)
            {
                result = result ^ (hash_value(field) << 1);
            }

            return result;
        }

        if (std::holds_alternative<ArrayPtr>(key))
        {
            auto value = std::get<ArrayPtr>(key);
            size_t result = 0;

            for (int64_t i = 0; i < value->size(); ++i)
            {
                result = result ^ (hash_value((*value)[i]) << 1);
            }

            return result;
        }

        if (std::holds_alternative<HashTablePtr>(key))
        {
            auto value = std::get<HashTablePtr>(key);
            size_t result = 0;

            for (auto iter : value->get_items())
            {
                result = result ^ (hash_value(iter.first) << 1) ^ (hash_value(iter.second) << 1);
            }

            return result;
        }

        if (std::holds_alternative<Token>(key))
        {
            auto value = std::get<Token>(key);

            size_t result = std::hash<int>()((int)value.type);
            result = result ^ (hash_value(value.location) << 1);
            result = result ^ (std::hash<std::string>()(value.text) << 1);

            return result;

        }

        if (std::holds_alternative<Location>(key))
        {
            auto value = std::get<Location>(key);

            size_t result = std::hash<std::string>()(value.get_path()->string());
            result = result ^ (std::hash<size_t>()(value.get_line()) << 1);
            result = result ^ (std::hash<size_t>()(value.get_column()) << 1);
        }

        if (std::holds_alternative<ByteCode>(key))
        {
            auto value = std::get<ByteCode>(key);
            size_t result = 0;

            for (auto& op : value)
            {
                result = result ^ (std::hash<size_t>()((size_t)op.id) << 1);
                result = result ^ (std::hash<Value>()(op.value) << 1);
            }

            return result;
        }

        if (std::holds_alternative<std::thread::id>(key))
        {
            auto value = std::get<std::thread::id>(key);

            return hash_value(value);
        }

        return 0;
    }


    bool operator ==(const sorth::Value& rhs, const sorth::Value& lhs)
    {
        if (rhs.index() != lhs.index())
        {
            return false;
        }

        if (std::holds_alternative<int64_t>(rhs))
        {
            return std::get<int64_t>(rhs) == std::get<int64_t>(lhs);
        }

        if (std::holds_alternative<double>(rhs))
        {
            return std::get<double>(rhs) == std::get<double>(lhs);
        }

        if (std::holds_alternative<std::string>(rhs))
        {
            return std::get<std::string>(rhs) == std::get<std::string>(lhs);
        }

        if (std::holds_alternative<DataObjectPtr>(rhs))
        {
            return (std::get<DataObjectPtr>(rhs) == std::get<DataObjectPtr>(lhs));
        }

        if (std::holds_alternative<ArrayPtr>(rhs))
        {
            return std::get<ArrayPtr>(rhs) == std::get<ArrayPtr>(lhs);
        }

        if (std::holds_alternative<HashTablePtr>(rhs))
        {
            return std::get<HashTablePtr>(rhs) == std::get<HashTablePtr>(lhs);
        }

        if (std::holds_alternative<Token>(rhs))
        {
            return std::get<Token>(rhs) == std::get<Token>(lhs);
        }

        if (std::holds_alternative<Location>(rhs))
        {
            return std::get<Location>(rhs) == std::get<Location>(lhs);
        }

        if (std::holds_alternative<ByteCode>(rhs))
        {
            return std::get<ByteCode>(rhs) == std::get<ByteCode>(lhs);
        }

        if (std::holds_alternative<std::thread::id>(rhs))
        {
            return std::get<std::thread::id>(lhs) == std::get<std::thread::id>(rhs);
        }

        return false;
    }


    Value deep_copy_value(InterpreterPtr& interpreter, const Value& value)
    {
        if (   std::holds_alternative<int64_t>(value)
            || std::holds_alternative<double>(value)
            || std::holds_alternative<bool>(value)
            || std::holds_alternative<std::string>(value)
            || std::holds_alternative<internal::Token>(value)
            || std::holds_alternative<internal::Location>(value)
            || std::holds_alternative<internal::ByteCode>(value))
        {
            return value;
        }

        if (std::holds_alternative<DataObjectPtr>(value))
        {
            return deep_copy_data_object(interpreter, value);
        }

        if (std::holds_alternative<ArrayPtr>(value))
        {
            return deep_copy_array(interpreter, value);
        }

        if (std::holds_alternative<ByteBufferPtr>(value))
        {
            return deep_copy_byte_buffer(interpreter, value);
        }

        if (std::holds_alternative<HashTablePtr>(value))
        {
            return deep_copy_hash_table(interpreter, value);
        }

        if (std::holds_alternative<std::thread::id>(value))
        {
            return std::get<std::thread::id>(value);
        }

        throw_error(*interpreter, "Deep copy of unexpected type.");
    }



    Value deep_copy_data_object(InterpreterPtr& interpreter, const Value& value)
    {
        auto original = std::get<DataObjectPtr>(value);
        auto new_object = std::make_shared<DataObject>();

        new_object->definition = original->definition;
        new_object->fields.resize(original->fields.size());

        for (size_t i = 0; i < original->fields.size(); ++i)
        {
            new_object->fields[i] = deep_copy_value(interpreter, original->fields[i]);
        }

        return new_object;
    }


    Value deep_copy_array(InterpreterPtr& interpreter, const Value& value)
    {
        auto original = std::get<ArrayPtr>(value);
        auto new_object = std::make_shared<Array>(original->size());

        for (size_t i = 0; i < (size_t)original->size(); ++i)
        {
            (*new_object)[i] = deep_copy_value(interpreter, (*original)[i]);
        }

        return new_object;
    }


    Value deep_copy_byte_buffer(InterpreterPtr& interpreter, const Value& value)
    {
        auto original = std::get<ByteBufferPtr>(value);
        auto new_object = std::make_shared<ByteBuffer>(original->size());

        memcpy(new_object->data_ptr(), original->data_ptr(), original->size());

        return new_object;
    }


    Value deep_copy_hash_table(InterpreterPtr& interpreter, const Value& value)
    {
        auto original = std::get<HashTablePtr>(value);
        auto new_object = std::make_shared<HashTable>();

        for (auto entry : original->get_items())
        {
            auto key = entry.first;
            auto value = entry.second;
            new_object->insert(deep_copy_value(interpreter, key),
                               deep_copy_value(interpreter, value));
        }

        return new_object;
    }


}
