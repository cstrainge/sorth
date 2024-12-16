
#pragma once


namespace sorth::internal
{


    struct Instruction
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
            mark_context,
            release_context,
            jump,
            jump_if_zero,
            jump_if_not_zero,
            jump_loop_start,
            jump_loop_exit,
            jump_target
        };

        Id id;
        Value value;
        std::optional<Location> location;
    };


    using ByteCode = std::vector<Instruction>;


    std::ostream& operator <<(std::ostream& stream, const Instruction::Id id);
    std::ostream& operator <<(std::ostream& stream, const Instruction& op);
    std::ostream& operator <<(std::ostream& stream, const ByteCode& code);


    bool operator ==(const Instruction& rhs, const Instruction& lhs);

    bool operator ==(const ByteCode& rhs, const ByteCode& lhs);


}
