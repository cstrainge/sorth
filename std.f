
: variable immediate word op.def_variable ;

: @ immediate op.read_variable ;
: ! immediate op.write_variable ;


: const immediate word op.def_constant ;


: if immediate
    variable else_label
    unique_str else_label !

    variable end_label
    unique_str end_label !

    code.new_block

    else_label @ op.jump_if_zero
    "else" 1 code.compile_until_words
    drop

    end_label @ op.jump

    else_label @ op.jump_target

    "then" 1 code.compile_until_words
    drop

    end_label @ op.jump_target

    code.resolve_jumps
    code.merge_stack_block
;


: if immediate
    variable if_fail_label
    unique_str if_fail_label !

    code.new_block

    if_fail_label @ op.jump_if_zero

    "else" "then" 2 code.compile_until_words

    "then" =
    if
        if_fail_label @ op.jump_target
    else
        variable then_label
        unique_str then_label !

        then_label @ op.jump

        if_fail_label @ op.jump_target


        "then" 1 code.compile_until_words
        drop

        then_label @ op.jump_target
    then

    code.resolve_jumps
    code.merge_stack_block
;


: .cr . cr ;
: ? @ .cr ;

: q    quit ;
: exit quit ;
