
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void string_or_numeric_op(InterpreterPtr& interpreter,
                                  std::function<void(double, double)> dop,
                                  std::function<void(int64_t, int64_t)> iop,
                                  std::function<void(std::string, std::string)> sop)
        {
            auto b = interpreter->pop();
            auto a = interpreter->pop();

            if (Value::either_is_string(a, b))
            {
                sop(a.as_string_with_conversion(), b.as_string_with_conversion());
            }
            else if (Value::either_is_float(a, b))
            {
                dop(a.as_float(interpreter), b.as_float(interpreter));
            }
            else if (Value::either_is_numeric(a, b))
            {
                iop(a.as_integer(interpreter), b.as_integer(interpreter));
            }
            else
            {
                throw_error(interpreter, "Expected string or numeric values.");
            }
        }


        void math_op(InterpreterPtr& interpreter,
                     std::function<double(double, double)> dop,
                     std::function<int64_t(int64_t, int64_t)> iop)
        {
            Value b = interpreter->pop();
            Value a = interpreter->pop();
            Value result;

            if (Value::either_is_float(a, b))
            {
                result = dop(a.as_float(interpreter), b.as_float(interpreter));
            }
            else if (Value::either_is_integer(a, b))
            {
                result = iop(a.as_integer(interpreter), b.as_integer(interpreter));
            }
            else
            {
                throw_error(interpreter, "Expected numeric values.");
            }

            interpreter->push(result);
        }


        void logic_op(InterpreterPtr& interpreter, std::function<bool(bool, bool)> op)
        {
            auto b = interpreter->pop_as_bool();
            auto a = interpreter->pop_as_bool();

            auto result = op(a, b);

            interpreter->push(result);
        }


        void logic_bit_op(InterpreterPtr& interpreter, std::function<int64_t(int64_t, int64_t)> op)
        {
            auto b = interpreter->pop_as_integer();
            auto a = interpreter->pop_as_integer();

            auto result = op(a, b);

            interpreter->push(result);
        }


        void word_add(InterpreterPtr& interpreter)
        {
            string_or_numeric_op(interpreter,
                                [&](auto a, auto b) { interpreter->push(a + b); },
                                [&](auto a, auto b) { interpreter->push(a + b); },
                                [&](auto a, auto b) { interpreter->push(a + b); });
        }


        void word_subtract(InterpreterPtr& interpreter)
        {
            math_op(interpreter,
                    [](auto a, auto b) -> auto { return a - b; },
                    [](auto a, auto b) -> auto { return a - b; });
        }


        void word_multiply(InterpreterPtr& interpreter)
        {
            math_op(interpreter,
                    [](auto a, auto b) -> auto { return a * b; },
                    [](auto a, auto b) -> auto { return a * b; });
        }


        void word_divide(InterpreterPtr& interpreter)
        {
            math_op(interpreter,
                    [](auto a, auto b) -> auto { return a / b; },
                    [](auto a, auto b) -> auto { return a / b; });
        }


        void word_mod(InterpreterPtr& interpreter)
        {
            auto b = interpreter->pop_as_integer();
            auto a = interpreter->pop_as_integer();

            interpreter->push(a % b);
        }


        void word_logic_and(InterpreterPtr& interpreter)
        {
            logic_op(interpreter, [](auto a, auto b) { return a && b; });
        }


        void word_logic_or(InterpreterPtr& interpreter)
        {
            logic_op(interpreter, [](auto a, auto b) { return a || b; });
        }


        void word_logic_not(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop_as_bool();

            interpreter->push(!value);
        }


        void word_bit_and(InterpreterPtr& interpreter)
        {
            logic_bit_op(interpreter, [](auto a, auto b) { return a & b; });
        }


        void word_bit_or(InterpreterPtr& interpreter)
        {
            logic_bit_op(interpreter, [](auto a, auto b) { return a | b; });
        }


        void word_bit_xor(InterpreterPtr& interpreter)
        {
            logic_bit_op(interpreter, [](auto a, auto b) { return a ^ b; });
        }


        void word_bit_not(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop_as_integer();

            value = ~value;

            interpreter->push(value);
        }


        void word_bit_left_shift(InterpreterPtr& interpreter)
        {
            logic_bit_op(interpreter, [](auto value, auto amount) { return value << amount; });
        }


        void word_bit_right_shift(InterpreterPtr& interpreter)
        {
            logic_bit_op(interpreter, [](auto value, auto amount) { return value >> amount; });
        }


        void word_equal(InterpreterPtr& interpreter)
        {
            string_or_numeric_op(interpreter,
                                [&](auto a, auto b) { interpreter->push((bool)(a == b)); },
                                [&](auto a, auto b) { interpreter->push((bool)(a == b)); },
                                [&](auto a, auto b) { interpreter->push((bool)(a == b)); });
        }


        void word_greater_equal(InterpreterPtr& interpreter)
        {
            string_or_numeric_op(interpreter,
                                [&](auto a, auto b) { interpreter->push((bool)(a >= b)); },
                                [&](auto a, auto b) { interpreter->push((bool)(a >= b)); },
                                [&](auto a, auto b) { interpreter->push((bool)(a >= b)); });
        }


        void word_less_equal(InterpreterPtr& interpreter)
        {
            string_or_numeric_op(interpreter,
                                [&](auto a, auto b) { interpreter->push((bool)(a <= b)); },
                                [&](auto a, auto b) { interpreter->push((bool)(a <= b)); },
                                [&](auto a, auto b) { interpreter->push((bool)(a <= b)); });
        }


        void word_greater(InterpreterPtr& interpreter)
        {
            string_or_numeric_op(interpreter,
                                [&](auto a, auto b) { interpreter->push((bool)(a > b)); },
                                [&](auto a, auto b) { interpreter->push((bool)(a > b)); },
                                [&](auto a, auto b) { interpreter->push((bool)(a > b)); });
        }


        void word_less(InterpreterPtr& interpreter)
        {
            string_or_numeric_op(interpreter,
                                [&](auto a, auto b) { interpreter->push((bool)(a < b)); },
                                [&](auto a, auto b) { interpreter->push((bool)(a < b)); },
                                [&](auto a, auto b) { interpreter->push((bool)(a < b)); });
        }


    }


    void register_math_logic_words(InterpreterPtr& interpreter)
    {
        // Math ops.
        ADD_NATIVE_WORD(interpreter, "+", word_add,
            "Add 2 numbers or strings together.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "-", word_subtract,
            "Subtract 2 numbers.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "*", word_multiply,
            "Multiply 2 numbers.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "/", word_divide,
            "Divide 2 numbers.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "%", word_mod,
            "Mod 2 numbers.",
            "a b -- result");


        // Logical words.
        ADD_NATIVE_WORD(interpreter, "&&", word_logic_and,
            "Logically compare 2 values.",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, "||", word_logic_or,
            "Logically compare 2 values.",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, "'", word_logic_not,
            "Logically invert a boolean value.",
            "bool -- bool");


        // Bitwise operator words.
        ADD_NATIVE_WORD(interpreter, "&", word_bit_and,
            "Bitwise AND two numbers together.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "|", word_bit_or,
            "Bitwise OR two numbers together.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "^", word_bit_xor,
            "Bitwise XOR two numbers together.",
            "a b -- result");

        ADD_NATIVE_WORD(interpreter, "~", word_bit_not,
            "Bitwise NOT a number.",
            "number -- result");

        ADD_NATIVE_WORD(interpreter, "<<", word_bit_left_shift,
            "Shift a numbers bits to the left.",
            "value amount -- result");

        ADD_NATIVE_WORD(interpreter, ">>", word_bit_right_shift,
            "Shift a numbers bits to the right.",
            "value amount -- result");


        // Equality words.
        ADD_NATIVE_WORD(interpreter, "=", word_equal,
            "Are 2 values equal?",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, ">=", word_greater_equal,
            "Is one value greater or equal to another?",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, "<=", word_less_equal,
            "Is one value less than or equal to another?",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, ">", word_greater,
            "Is one value greater than another?",
            "a b -- bool");

        ADD_NATIVE_WORD(interpreter, "<", word_less,
            "Is one value less than another?",
            "a b -- bool");
    }


}
