
: variable immediate word op.def_variable ;

: @ immediate op.read_variable ;
: ! immediate op.write_variable ;


: constant immediate word op.def_constant ;


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


: ( immediate begin word ")" = until ;


( Now that we've defined comments, we can begin to document the code.  So far we've defined a few  )
( base words.  Thier implmentations are to simply generate an instruction that will perform their  )
( function into the bytecode stream being generated. )

( Next we define the 'if' statment.  The first one is just a basic version where else blocks are   )
( manditory.  Right after that we redefine 'if' to have a more flexible implementation.  Note that )
( we use the previous definition to assist us in creating the new one. )

( Building on that we define the two forms of the 'begin' loop. 'begin until' and 'begin repeat'.  )
( Once we had those things in place, we were able to define the comment block. )


( Case statement of the form:                                                                      )

(     case                                                                                         )
(         <test> of                                                                                )
(             <body>                                                                               )
(             endof                                                                                )
(                                                                                                  )
(         <test> of                                                                                )
(             <body>                                                                               )
(             endof                                                                                )
(         ...                                                                                      )
(                                                                                                  )
(         <default body>                                                                           )
(     endcase                                                                                      )

( Where it's expected to have an input value left on the stack, and each test block generates a    )
( value that's compared against that input for equality. )

: case immediate
    variable done
    false done !

    ( Label marking end of the entire case statement. )
    variable end_label
    unique_str end_label !

    ( Label for the next of statement test or the beginning of the default block. )
    variable next_label
    unique_str next_label !

    ( We create 2 code blocks on the construction stack.  The top one will hold the current case )
    ( or default block.  The bottom one is where we will consolidate everything for final        )
    ( resolution back into the base block we are generating for. )
    code.new_block
    code.new_block

    begin
        ( Ok, compile either the case of test or the end case block.  We won't know for sure which )
        ( it is until we hit the next keyword. )
        "of" "endcase" 2 code.compile_until_words

        "of" =
        if
            ( We've just compiled the case of test.  We need to duplicate and preserve the input )
            ( value before the test code burns it up.  So, insert the call to dup at the         )
            ( begginning of the currrent code block. )
            true code.insert_at_front
            ` dup op.execute
            false code.insert_at_front

            ( Now, check to see if the value the case test left on the stack is equal to the input )
            ( we were given before the case statement began executing. )
            ` = op.execute

            ( If the test fails, jump to the next case test.  If it succedes we drop the input )
            ( value as it isn't needed anymore. )
            next_label @ op.jump_if_zero
            ` drop op.execute

            ( Compile the body of the case block itself. )
            "endof" 1 code.compile_until_words
            drop

            ( Once the block is done executing, jump to the end of the whole statement. )
            end_label @ op.jump

            ( Now that we're outside of the case block, we can mark the beginning of the next one. )
            ( Note that we also generate a new unique id for the next case block, should we find   )
            ( one. )
            next_label @ op.jump_target
            unique_str next_label !

            ( Merge this block into the base one, and create a new one for the next section we )
            ( find. )
            code.merge_stack_block
            code.new_block
        else
            ( Looks like we've found the default case block.  Again, we need to insert an          )
            ( instruction before the user code.  In this case it's to drop the input value as it's )
            ( not needed anymore. )
            true code.insert_at_front
            ` drop op.execute
            false code.insert_at_front

            ( We can now mark a jump target for the end of the entire case statement.  We also )
            ( note that we are done with the loop here. )
            end_label @ op.jump_target
            true done !

            ( Merge the last block into the base code. )
            code.merge_stack_block
        then

        ( A simple loop until done loop. )
        done @
    until

    ( Ok, resolve all of the jump symbols and merge this block back into the base code being )
    ( compiled by the interpreter. )
    code.resolve_jumps
    code.merge_stack_block
;


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


( Helper words for reading/writing byte buffers. )

: buffer.i8!!  @ 1 buffer.int! ;
: buffer.i16!! @ 2 buffer.int! ;
: buffer.i32!! @ 4 buffer.int! ;
: buffer.i64!! @ 8 buffer.int! ;

: buffer.i8@@  @ 1 true buffer.int@ ;
: buffer.i16@@ @ 2 true buffer.int@ ;
: buffer.i32@@ @ 4 true buffer.int@ ;
: buffer.i64@@ @ 8 true buffer.int@ ;

: buffer.u8@@  @ 1 false buffer.int@ ;
: buffer.u16@@ @ 2 false buffer.int@ ;
: buffer.u32@@ @ 4 false buffer.int@ ;
: buffer.u64@@ @ 8 false buffer.int@ ;

: buffer.f32!! @ 4 buffer.float! ;
: buffer.f64!! @ 8 buffer.float! ;

: buffer.f32@@ @ 4 buffer.float@ ;
: buffer.f64@@ @ 8 buffer.float@ ;

: buffer.string!! @ swap buffer.string! ;
: buffer.string@@ @ swap buffer.string@ ;

: buffer.position!! @ buffer.position! ;
: buffer.position@@ @ buffer.position@ ;


( Define a user prompt for the REPL. )

: prompt "\027[2:34m>\027[0:0m>" . ;


( Quick hack to let scripts be executable from the command line. )

: #!/usr/bin/env ;
: sorth ;
