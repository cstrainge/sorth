
#pragma once


namespace sorth::internal
{


    struct OperationCode
    {
        enum class Id : unsigned char
        {
            def_variable,
            def_constant,
            read_variable,
            write_variable,
            execute,
            word_index,
            push_constant_value,
            mark_catch,
            jump,
            jump_if_zero,
            jump_if_not_zero,
            jump_target
        };

        Id id;
        Value value;
        std::optional<Location> location;
    };


    std::ostream& operator <<(std::ostream& stream, const OperationCode::Id id);
    std::ostream& operator <<(std::ostream& stream, const OperationCode& op);
    std::ostream& operator <<(std::ostream& stream, const ByteCode& code);


    struct CallItem
    {
        Location word_location;
        std::string word_name;
    };


    using CallStack = std::list<CallItem>;


    std::ostream& operator <<(std::ostream& stream, const CallStack& call_stack);


}
