
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



: .cr . cr ;
: q    quit ;
: exit quit ;
