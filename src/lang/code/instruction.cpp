
#include "sorth.h"



namespace sorth::internal
{


    std::ostream& operator <<(std::ostream& stream, const Instruction::Id id)
    {
        switch (id)
        {
            case Instruction::Id::def_variable:        stream << "def_variable       "; break;
            case Instruction::Id::def_constant:        stream << "def_constant       "; break;
            case Instruction::Id::read_variable:       stream << "read_variable      "; break;
            case Instruction::Id::write_variable:      stream << "write_variable     "; break;
            case Instruction::Id::execute:             stream << "execute            "; break;
            case Instruction::Id::word_index:          stream << "word_index         "; break;
            case Instruction::Id::word_exists:         stream << "word_exists        "; break;
            case Instruction::Id::push_constant_value: stream << "push_constant_value"; break;
            case Instruction::Id::mark_loop_exit:      stream << "mark_loop_exit     "; break;
            case Instruction::Id::unmark_loop_exit:    stream << "unmark_loop_exit   "; break;
            case Instruction::Id::mark_catch:          stream << "mark_catch         "; break;
            case Instruction::Id::unmark_catch:        stream << "unmark_catch       "; break;
            case Instruction::Id::mark_context:        stream << "mark_context       "; break;
            case Instruction::Id::release_context:     stream << "release_context    "; break;
            case Instruction::Id::jump:                stream << "jump               "; break;
            case Instruction::Id::jump_if_zero:        stream << "jump_if_zero       "; break;
            case Instruction::Id::jump_if_not_zero:    stream << "jump_if_not_zero   "; break;
            case Instruction::Id::jump_loop_start:     stream << "jump_loop_start    "; break;
            case Instruction::Id::jump_loop_exit:      stream << "jump_loop_exit     "; break;
            case Instruction::Id::jump_target:         stream << "jump_target        "; break;
        }

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const Instruction& op)
    {
        auto doesnt_have_parameter = [](Instruction::Id id, const Value& value)
            {
                if (id == Instruction::Id::jump_target)
                {
                    return value.is_string() ? false : true;
                }

                return    (id == Instruction::Id::read_variable)
                       || (id == Instruction::Id::write_variable)
                       || (id == Instruction::Id::unmark_loop_exit)
                       || (id == Instruction::Id::unmark_catch)
                       || (id == Instruction::Id::mark_context)
                       || (id == Instruction::Id::release_context)
                       || (id == Instruction::Id::jump_loop_exit);
            };

        stream << op.id;

        if (!doesnt_have_parameter(op.id, op.value))
        {
            if (   (op.id == Instruction::Id::push_constant_value)
                && (op.value.is_string()))
            {
                stream << "  " << stringify(op.value.as_string_with_conversion());
            }
            else
            {
                stream << "  " << op.value;
            }
        }

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const ByteCode& code)
    {
        auto is_jump_with_value = [](Instruction::Id id)
            {
                return    (id == Instruction::Id::jump)
                       || (id == Instruction::Id::jump_if_zero)
                       || (id == Instruction::Id::jump_if_not_zero)
                       || (id == Instruction::Id::mark_loop_exit)
                       || (id == Instruction::Id::mark_catch);
            };

        for (size_t i = 0; i < code.size(); ++i)
        {
            const auto& op = code[i];
            stream << std::setw(6) << i << "  ";

            if (!is_jump_with_value(op.id))
            {
                stream << op << std::endl;
            }
            else
            {
                stream << op.id << "  " << op.value << std::endl;
            }
        }

        return stream;
    }


    bool operator ==(const Instruction& rhs, const Instruction& lhs)
    {
        if (rhs.id != lhs.id)
        {
            return false;
        }

        // We ignore location, if the op and value are the same, the code is considered so too.

        if (rhs.value != lhs.value)
        {
            return false;
        }

        return true;
    }


    bool operator ==(const ByteCode& rhs, const ByteCode& lhs)
    {
        if (rhs.size() != lhs.size())
        {
            return false;
        }

        for (size_t i = 0; i < rhs.size(); ++i)
        {
            if (!(rhs[i] == lhs[i]))
            {
                return false;
            }
        }

        return true;
    }


}
