
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void word_op_def_variable(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            if (value.is_token())
            {
                value = value.as_token(interpreter).text;
            }

            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::def_variable,
                    .value = value
                });
        }


        void word_op_def_constant(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            if (value.is_token())
            {
                value = value.as_token(interpreter).text;
            }

            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::def_constant,
                    .value = value
                });
        }


        void word_op_read_variable(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::read_variable
                });
        }


        void word_op_write_variable(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::write_variable
                });
        }


        void word_op_execute(InterpreterPtr& interpreter)
        {
            auto value = interpreter->pop();

            if (value.is_token())
            {
                value = value.as_token(interpreter).text;
            }

            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::execute,
                    .value = value
                });
        }


        void word_op_push_constant_value(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::push_constant_value,
                    .value = interpreter->pop()
                });
        }


        void word_mark_loop_exit(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::mark_loop_exit,
                    .value = interpreter->pop()
                });
        }


        void word_unmark_loop_exit(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::unmark_loop_exit
                });
        }


        void word_op_mark_catch(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::mark_catch,
                    .value = interpreter->pop()
                });
        }


        void word_op_unmark_catch(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::unmark_catch
                });
        }


        void word_op_mark_context(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::mark_context
                });
        }


        void word_op_release_context(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::release_context
                });
        }


        void word_op_jump(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::jump,
                    .value = interpreter->pop()
                });
        }


        void word_op_jump_if_zero(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::jump_if_zero,
                    .value = interpreter->pop()
                });
        }


        void word_op_jump_if_not_zero(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::jump_if_not_zero,
                    .value = interpreter->pop()
                });
        }


        void word_jump_loop_start(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::jump_loop_start
                });
        }


        void word_jump_loop_exit(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::jump_loop_exit
                });
        }


        void word_op_jump_target(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().insert_instruction(
                {
                    .id = Instruction::Id::jump_target,
                    .value = interpreter->pop()
                });
        }


        void word_code_new_block(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().new_construction();
        }


        void word_code_drop_stack_block(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().drop_construction();
        }


        void word_code_execute_stack_block(InterpreterPtr& interpreter)
        {
            auto name = interpreter->pop_as_string();
            interpreter->execute_code(name, interpreter->compile_context().construction().code);
        }


        void word_code_merge_stack_block(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().merge_construction();
        }


        void word_code_pop_stack_block(InterpreterPtr& interpreter)
        {
            interpreter->push(interpreter->compile_context().drop_construction().code);
        }


        void word_code_push_stack_block(InterpreterPtr& interpreter)
        {
            auto top = interpreter->pop_as_byte_code();

            interpreter->compile_context().new_construction(std::move(top));
        }


        void word_code_stack_block_size(InterpreterPtr& interpreter)
        {
            size_t size = interpreter->compile_context().construction().code.size();

            interpreter->push(static_cast<int64_t>(size));
        }


        void word_code_resolve_jumps(InterpreterPtr& interpreter)
        {
            auto is_jump = [](const Instruction& code) -> bool
                {
                    return    (code.id == Instruction::Id::jump)
                           || (code.id == Instruction::Id::jump_if_not_zero)
                           || (code.id == Instruction::Id::jump_if_zero)
                           || (code.id == Instruction::Id::mark_loop_exit)
                           || (code.id == Instruction::Id::mark_catch);
                };

            auto& top_code = interpreter->compile_context().construction().code;

            std::list<size_t> jump_indicies;
            std::unordered_map<std::string, size_t> jump_targets;

            for (size_t i = 0; i < top_code.size(); ++i)
            {
                if (   is_jump(top_code[i])
                    && (top_code[i].value.is_string()))
                {
                    jump_indicies.push_back(i);
                }
                else if (   (top_code[i].id == Instruction::Id::jump_target)
                        && (top_code[i].value.is_string()))
                {
                    jump_targets.insert({ top_code[i].value.as_string(interpreter), i });
                    top_code[i].value = (int64_t)0;
                }
            }

            for (auto jump_index : jump_indicies)
            {
                auto& jump_op = top_code[jump_index];

                auto jump_name = jump_op.value.as_string(interpreter);
                auto iter = jump_targets.find(jump_name);

                if (iter != jump_targets.end())
                {
                    auto target_index = iter->second;
                    jump_op.value = (int64_t)target_index - (int64_t)jump_index;
                }
            }
        }


        void word_code_compile_until_words(InterpreterPtr& interpreter)
        {
            auto count = interpreter->pop_as_integer();
            std::vector<std::string> word_list;

            for (int64_t i = 0; i < count; ++i)
            {
                word_list.push_back(interpreter->pop_as_string());
            }

            auto found = interpreter->compile_context().compile_until_words(word_list);

            interpreter->push(found);
        }


        void word_code_insert_at_front(InterpreterPtr& interpreter)
        {
            interpreter->compile_context().set_insertion(  interpreter->pop_as_bool()
                                                         ? CodeInsertionPoint::beginning
                                                         : CodeInsertionPoint::end);
        }


        void word_code_execute_source(InterpreterPtr& interpreter)
        {
            auto name = interpreter->pop_as_string();
            auto code = interpreter->pop_as_string();

            interpreter->process_source(name, code);
        }


    }


    void register_bytecode_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "op.def_variable", word_op_def_variable,
            "Insert this instruction into the byte stream.",
            "new-name -- ");

        ADD_NATIVE_WORD(interpreter, "op.def_constant", word_op_def_constant,
            "Insert this instruction into the byte stream.",
            "new-name -- ");

        ADD_NATIVE_WORD(interpreter, "op.read_variable", word_op_read_variable,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.write_variable", word_op_write_variable,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.execute", word_op_execute,
            "Insert this instruction into the byte stream.",
            "index -- ");

        ADD_NATIVE_WORD(interpreter, "op.push_constant_value", word_op_push_constant_value,
            "Insert this instruction into the byte stream.",
            "value -- ");

        ADD_NATIVE_WORD(interpreter, "op.mark_loop_exit", word_mark_loop_exit,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "op.unmark_loop_exit", word_unmark_loop_exit,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.mark_catch", word_op_mark_catch,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "op.unmark_catch", word_op_unmark_catch,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.mark_context", word_op_mark_context,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.release_context", word_op_release_context,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump", word_op_jump,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump_if_zero", word_op_jump_if_zero,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump_if_not_zero", word_op_jump_if_not_zero,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump_loop_start", word_jump_loop_start,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump_loop_exit", word_jump_loop_exit,
            "Insert this instruction into the byte stream.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "op.jump_target", word_op_jump_target,
            "Insert this instruction into the byte stream.",
            "identifier -- ");

        ADD_NATIVE_WORD(interpreter, "code.new_block", word_code_new_block,
            "Create a new sub-block on the code generation stack.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "code.drop_stack_block", word_code_drop_stack_block,
            "Drop the code object that's at the top of the construction stack.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "code.execute_stack_block", word_code_execute_stack_block,
            "Take the top code block of the current construction and execute it now.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "code.merge_stack_block", word_code_merge_stack_block,
            "Merge the top code block into the one below.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "code.pop_stack_block", word_code_pop_stack_block,
            "Pop a code block off of the code stack and onto the data stack.",
            " -- code_block");

        ADD_NATIVE_WORD(interpreter, "code.push_stack_block", word_code_push_stack_block,
            "Pop a block from the data stack and back onto the code stack.",
            "code_block -- ");

        ADD_NATIVE_WORD(interpreter, "code.stack-block-size@", word_code_stack_block_size,
            "Get the size of the top code block on the stack.",
            " -- size");

        ADD_NATIVE_WORD(interpreter, "code.resolve_jumps", word_code_resolve_jumps,
            "Resolve all of the jumps in the top code block.",
            " -- ");

        ADD_NATIVE_WORD(interpreter, "code.compile_until_words", word_code_compile_until_words,
            "Compile words until one of the given words is found.",
            "words... word_count -- found_word");

        ADD_NATIVE_WORD(interpreter, "code.insert_at_front", word_code_insert_at_front,
            "When true new instructions are added beginning of the block.",
            "bool -- ");

        ADD_NATIVE_WORD(interpreter, "code.execute_source", word_code_execute_source,
            "Interpret and execute a string like it is source code.",
            "string_to_execute name -- ???");
    }


}
