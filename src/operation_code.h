
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
            word_exists,
            push_constant_value,
            mark_loop_exit,
            unmark_loop_exit,
            mark_catch,
            unmark_catch,
            jump,
            jump_if_zero,
            jump_if_not_zero,
            jump_loop_exit,
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
