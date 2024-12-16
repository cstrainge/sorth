
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void word_dup(InterpreterPtr& interpreter)
        {
            auto next = interpreter->pop();

            interpreter->push(next);
            interpreter->push(next);
        }


        void word_drop(InterpreterPtr& interpreter)
        {
            interpreter->pop();
        }


        void word_swap(InterpreterPtr& interpreter)
        {
            auto a = interpreter->pop();
            auto b = interpreter->pop();

            interpreter->push(a);
            interpreter->push(b);
        }


        void word_over(InterpreterPtr& interpreter)
        {
            auto a = interpreter->pop();
            auto b = interpreter->pop();

            interpreter->push(a);
            interpreter->push(b);
            interpreter->push(a);
        }


        void word_rot(InterpreterPtr& interpreter)
        {
            auto c = interpreter->pop();
            auto b = interpreter->pop();
            auto a = interpreter->pop();

            interpreter->push(c);
            interpreter->push(a);
            interpreter->push(b);
        }


        void word_stack_depth(InterpreterPtr& interpreter)
        {
            interpreter->push(interpreter->depth());
        }


        void word_max_stack_depth(InterpreterPtr& interpreter)
        {
            //interpreter->push(interpreter->stack_max_depth());
        }


        void word_pick(InterpreterPtr& interpreter)
        {
            auto index = interpreter->pop_as_integer();

            interpreter->push(interpreter->pick(index));
        }


        void word_push_to(InterpreterPtr& interpreter)
        {
            auto index = interpreter->pop_as_integer();
            interpreter->push_to(index);
        }


    }


    void register_stack_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "dup", word_dup,
            "Duplicate the top value on the data stack.",
            "value -- value value");

        ADD_NATIVE_WORD(interpreter, "drop", word_drop,
            "Discard the top value on the data stack.",
            "value -- ");

        ADD_NATIVE_WORD(interpreter, "swap", word_swap,
            "Swap the top 2 values on the data stack.",
            "a b -- b a");

        ADD_NATIVE_WORD(interpreter, "over", word_over,
            "Make a copy of the top value and place the copy under the second.",
            "a b -- b a b");

        ADD_NATIVE_WORD(interpreter, "rot", word_rot,
            "Rotate the top 3 values on the stack.",
            "a b c -- c a b");

        ADD_NATIVE_WORD(interpreter, "stack.depth", word_stack_depth,
            "Get the current depth of the stack.",
            " -- depth");

        ADD_NATIVE_WORD(interpreter, "stack.max-depth", word_max_stack_depth,
            "Get the maximum depth of the stack.",
            " -- depth");

        ADD_NATIVE_WORD(interpreter, "pick", word_pick,
            "Pick the value n locations down in the stack and push it on the top.",
            "n -- value");

        ADD_NATIVE_WORD(interpreter, "push-to", word_push_to,
            "Pop the top value and push it back into the stack a position from the top.",
            "n -- <updated-stack>>");
    }


}
