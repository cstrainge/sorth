
#include "sorth.h"



namespace sorth::internal
{


    std::ostream& operator <<(std::ostream& stream, const OperationCode::Id id)
    {
        switch (id)
        {
            case OperationCode::Id::def_variable:        stream << "def_variable       "; break;
            case OperationCode::Id::def_constant:        stream << "def_constant       "; break;
            case OperationCode::Id::read_variable:       stream << "read_variable      "; break;
            case OperationCode::Id::write_variable:      stream << "write_variable     "; break;
            case OperationCode::Id::execute:             stream << "execute            "; break;
            case OperationCode::Id::word_index:          stream << "word_index         "; break;
            case OperationCode::Id::word_exists:         stream << "word_exists        "; break;
            case OperationCode::Id::push_constant_value: stream << "push_constant_value"; break;
            case OperationCode::Id::mark_loop_exit:      stream << "mark_loop_exit     "; break;
            case OperationCode::Id::unmark_loop_exit:    stream << "unmark_loop_exit   "; break;
            case OperationCode::Id::mark_catch:          stream << "mark_catch         "; break;
            case OperationCode::Id::unmark_catch:        stream << "unmark_catch       "; break;
            case OperationCode::Id::jump:                stream << "jump               "; break;
            case OperationCode::Id::jump_if_zero:        stream << "jump_if_zero       "; break;
            case OperationCode::Id::jump_if_not_zero:    stream << "jump_if_not_zero   "; break;
            case OperationCode::Id::jump_loop_start:     stream << "jump_loop_start    "; break;
            case OperationCode::Id::jump_loop_exit:      stream << "jump_loop_exit     "; break;
            case OperationCode::Id::jump_target:         stream << "jump_target        "; break;
        }

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const OperationCode& op)
    {
        auto doesnt_have_parameter = [](OperationCode::Id id)
            {
                return    (id == OperationCode::Id::read_variable)
                       || (id == OperationCode::Id::write_variable)
                       || (id == OperationCode::Id::jump_target)
                       || (id == OperationCode::Id::unmark_loop_exit)
                       || (id == OperationCode::Id::unmark_catch)
                       || (id == OperationCode::Id::jump_loop_exit);
            };

        stream << op.id;

        if (!doesnt_have_parameter(op.id))
        {
            stream << "  " << op.value;
        }

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const ByteCode& code)
    {
        for (size_t i = 0; i < code.size(); ++i)
        {
            const auto& op = code[i];
            stream << std::setw(6) << i << "  " << op << std::endl;
        }

        return stream;
    }



    std::ostream& operator <<(std::ostream& stream, const CallStack& call_stack)
    {
        stream << std::endl;

        for (const auto& item : call_stack)
        {
            stream << "    " << item.word_location << " -- " << item.word_name << std::endl;
        }

        return stream;
    }


}
