
#include "sorth.h"



namespace sorth::internal
{


    void pretty_print_bytecode(InterpreterPtr& interpreter,
                               const ByteCode& code,
                               std::ostream& stream)
    {
        for (size_t i = 0; i < code.size(); ++i)
        {
            stream << std::setw(6) << i << "  ";

            if (   (code[i].id == Instruction::Id::execute)
                && (code[i].value.is_numeric()))
            {
                auto index = code[i].value.as_integer(interpreter);
                auto word = interpreter->get_handler_info(index);

                stream << code[i].id << "  "
                       << word.name << ", (" << index << ")"
                       << std::endl;
            }
            else if (   (code[i].id == Instruction::Id::push_constant_value)
                        && (code[i].value.is_string()))
            {
                auto string = code[i].value.as_string(interpreter);

                stream << code[i].id << "  " << stringify(string) << std::endl;
            }
            else if (  (   (code[i].id == Instruction::Id::mark_loop_exit)
                        || (code[i].id == Instruction::Id::mark_catch)
                        || (code[i].id == Instruction::Id::jump)
                        || (code[i].id == Instruction::Id::jump_if_zero)
                        || (code[i].id == Instruction::Id::jump_if_not_zero))
                        && (code[i].value.is_numeric()))
            {
                auto offset = code[i].value.as_integer(interpreter);

                stream << code[i].id << "  " << i + offset << std::endl;
            }
            else
            {
                stream << code[i] << std::endl;
            }
        }
    }



    WordFunction::WordFunction()
    {
    }

    WordFunction::WordFunction(const Handler& function)
    :   function(function)
    {
    }

    WordFunction::WordFunction(const WordFunction& word_function)
    :   function(word_function.function),
        byte_code(word_function.byte_code),
        ir(word_function.ir),
        asm_code(word_function.asm_code)
    {
    }

    WordFunction::WordFunction(WordFunction&& word_function)
    :   function(std::move(word_function.function)),
        byte_code(std::move(word_function.byte_code)),
        ir(std::move(word_function.ir)),
        asm_code(std::move(word_function.asm_code))
    {
    }

    WordFunction::~WordFunction()
    {
    }

    WordFunction& WordFunction::operator =(const Handler& raw_function)
    {
        function = raw_function;

        return *this;
    }

    WordFunction& WordFunction::operator =(const WordFunction& word_function)
    {
        function = word_function.function;
        byte_code = word_function.byte_code;
        ir = word_function.ir;
        asm_code = word_function.asm_code;

        return *this;
    }

    WordFunction& WordFunction::operator =(WordFunction&& word_function)
    {
        function = std::move(word_function.function);
        byte_code = std::move(word_function.byte_code);
        ir = std::move(word_function.ir);
        asm_code = std::move(word_function.asm_code);

        return *this;
    }

    void WordFunction::operator ()(InterpreterPtr& interpreter)
    {
        function(interpreter);
    }

    WordFunction::Handler WordFunction::get_function() const
    {
        return function;
    }

    void WordFunction::set_byte_code(const ByteCode& code)
    {
        byte_code = code;
    }

    void WordFunction::set_byte_code(const ByteCode&& code)
    {
        byte_code = std::move(code);
    }

    const std::optional<ByteCode>& WordFunction::get_byte_code() const
    {
        return byte_code;
    }

    void WordFunction::set_ir(const std::string& code)
    {
        ir = code;
    }

    const std::optional<std::string>& WordFunction::get_ir() const
    {
        return ir;
    }

    void WordFunction::set_asm_code(const std::string& code)
    {
        asm_code = code;
    }

    const std::optional<std::string>& WordFunction::get_asm_code() const
    {
        return asm_code;
    }


}
