
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


: begin immediate
    variable top_label
    unique_str top_label !

    code.new_block

    top_label @ op.jump_target

    "while" "until" 2 code.compile_until_words

    "until" =
    if
        top_label @ op.jump_if_zero
    else
        variable end_label
        unique_str end_label !

        end_label @ op.jump_if_zero

        "repeat" 1 code.compile_until_words
        drop

        top_label @ op.jump
        end_label @ op.jump_target
    then

    code.resolve_jumps
    code.merge_stack_block
;


: case immediate
    variable done
    false done !

    variable end_label
    unique_str end_label !

    variable next_label
    unique_str next_label !

    code.new_block

    begin
        ` dup op.execute

        "of" "endcase" 2 code.compile_until_words

        "of" =
        if
            ` = op.execute
            next_label @ op.jump_if_zero

            "endof" 1 code.compile_until_words
            drop

            ` swap op.execute
            ` drop op.execute

            end_label @ op.jump

            next_label @ op.jump_target
            unique_str next_label !
        else
            ` swap op.execute
            ` drop op.execute

            ` swap op.execute
            ` drop op.execute

            end_label @ op.jump_target
            true done !
        then

        done @
    until

    code.resolve_jumps
    code.merge_stack_block
;


: ( immediate begin word ")" = until ;


( Now we can have comments. )

( Simple increment and decrements. )
: ++ ( value -- incremented ) 1 + ;
: -- ( value -- decremented ) 1 - ;


( Lets save some typing. )
: .cr  ( value -- ) . cr ;
: ?    ( value -- ) @ .cr ;
: .sp  ( count -- ) begin -- dup 0 >= while "" . repeat drop ;
: .hcr ( value -- ) .hex cr ;


( Handy comparisons. )
: 0>  ( value -- test_result ) 0 >  ;
: 0=  ( value -- test_result ) 0 =  ;
: 0<  ( value -- test_result ) 0 <  ;
: 0>= ( value -- test_result ) 0 >= ;
: 0<= ( value -- test_result ) 0 <= ;


( Increment and decrement variables. )
: +! ( value variable -- ) over @ + swap ! ;
: -! ( value variable -- ) over @ - swap ! ;


( Quicker data field access. )
: #!! @ #! ;
: #@@ @ #@ ;


( Alternate ways to exit the interpreter. )
: q    ( -- ) quit ;
: exit ( -- ) quit ;
