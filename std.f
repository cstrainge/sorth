
: variable immediate word op.def_variable ;

: @ immediate op.read_variable ;
: ! immediate op.write_variable ;

: variable! immediate
    word dup

    op.def_variable
    op.execute
    op.write_variable
;


: constant immediate word op.def_constant ;


: if immediate
    unique_str variable! else_label
    unique_str variable! end_label

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
    unique_str variable! if_fail_label

    code.new_block

    if_fail_label @ op.jump_if_zero

    "else" "then" 2 code.compile_until_words

    "then" =
    if
        if_fail_label @ op.jump_target
    else
        unique_str variable! then_label

        then_label @ op.jump

        if_fail_label @ op.jump_target

        "then" 1 code.compile_until_words
        drop

        then_label @ op.jump_target
    then

    code.resolve_jumps
    code.merge_stack_block
;


: break immediate op.jump_loop_exit ;
: continue immediate op.jump_loop_start ;


: begin immediate
    unique_str variable! top_label
    unique_str variable! end_label

    code.new_block

    end_label @ op.mark_loop_exit
    top_label @ op.jump_target

    "while" "until" 2 code.compile_until_words

    "until" =
    if
        top_label @ op.jump_if_zero
        end_label @ op.jump_target
        op.unmark_loop_exit
    else
        end_label @ op.jump_if_zero

        "repeat" 1 code.compile_until_words
        drop

        top_label @ op.jump
        end_label @ op.jump_target
        op.unmark_loop_exit
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
    false variable! done

    ( Label marking end of the entire case statement. )
    unique_str variable! case_end_label

    ( Label for the next of statement test or the beginning of the default block. )
    unique_str variable! next_label

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
            case_end_label @ op.jump

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
            case_end_label @ op.jump_target
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

: ++! ( variable -- ) dup @ ++ swap ! ;
: --! ( variable -- ) dup @ -- swap ! ;


( Make sure we have the regular printing words. )
: .    ( value -- ) term.! " " term.! ;

: .hex  ( value -- ) hex . ;

: cr   ( -- )       "\n" term.! term.flush ;
: .cr  ( value -- ) . cr ;
: .hcr ( value -- ) .hex cr ;

: ?    ( value -- ) @ .cr ;
: .sp  ( count -- ) begin -- dup 0 >= while "" . repeat drop ;


( Handy comparisons. )
: 0>  ( value -- test_result ) 0 >  ;
: 0=  ( value -- test_result ) 0 =  ;
: 0<  ( value -- test_result ) 0 <  ;
: 0>= ( value -- test_result ) 0 >= ;
: 0<= ( value -- test_result ) 0 <= ;


( Increment and decrement variables. )
: +! ( value variable -- ) over @ + swap ! ;
: -! ( value variable -- ) over @ - swap ! ;


( String variable words. )
: string.length@    ( string_var -- length )                  @ string.length    ;
: string.find@      ( search string_var -- position_or_npos ) @ string.find      ;
: string.to_number@ ( string_var -- new_number )              @ string.to_number ;


: string.insert! ( sub_string possition string_var -- )
    variable! var_index

    var_index @ @ string.insert
    var_index @ !
;


: string.remove! ( count position string_var -- )
    variable! var_index

    var_index @ @ string.remove
    var_index @ !
;


( Quicker data field access. )
: #!! @ #! ;
: #@@ @ #@ ;


( Array words. )
: []!! @ []! ;
: []@@ @ []@ ;

: [].resize! @ [].resize ;


: [ immediate
    1 variable! index_count
    1 [].new variable! index_blocks

    variable command

    false variable! found_end_bracket
    true variable! is_writing

    code.new_block

    begin
        "," "]!" "]!!" "]@" "]@@" 5 code.compile_until_words

        code.pop_stack_block index_count @ -- index_blocks []!!

        case
            "," of
                index_count ++!

                code.new_block
                index_count @ index_blocks [].resize!
            endof

            "]!"  of  ` []!  command !  true found_end_bracket !  endof
            "]!!" of  ` []!! command !  true found_end_bracket !  endof
            "]@"  of  ` []@  command !  true found_end_bracket !  false is_writing !  endof
            "]@@" of  ` []@@ command !  true found_end_bracket !  false is_writing !  endof
        endcase

        found_end_bracket @
    until

    index_count @ 1 =
    if
        0 index_blocks []@@ code.push_stack_block

        ` swap op.execute
        command @ op.execute

        code.merge_stack_block
    else
        index_count @ -- variable! i

        begin
            i @ index_blocks []@@ code.push_stack_block

            is_writing @
            if
                true code.insert_at_front
                ` over op.execute
                false code.insert_at_front
            else
                true code.insert_at_front
                ` dup op.execute
                false code.insert_at_front
            then

            ` swap op.execute
            command @ op.execute

            is_writing @ true <>
            if
                ` swap op.execute
            then

            code.merge_stack_block

            i --!
            i @ 0<
        until
        ` drop op.execute
    then
;


( A try/catch block for exception handling. )
: try immediate
    unique_str variable! catch_label
    unique_str variable! end_catch_label

    code.new_block

    catch_label @ op.mark_catch
    "catch" 1 code.compile_until_words
    drop

    op.unmark_catch
    end_catch_label @ op.jump

    catch_label @ op.jump_target
    "endcatch" 1 code.compile_until_words
    drop

    end_catch_label @ op.jump_target

    code.resolve_jumps
    code.merge_stack_block
;


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


( Allow user code to register an at exit handler. )
: at_exit immediate
    word op.push_constant_value
    ` at_exit op.execute
;


( Some useful words when dealing with the terminal. )
"\027"         constant term.esc  ( Terminal escape character. )
term.esc "[" + constant term.csi  ( Control sequence introducer. )


( Read from the terminal and expect it to be a specific character.  If it isn't a match an )
( exception is thrown. )
: term.expect_key  ( extpected_key -- )
    variable! expected
    term.key variable! got

    expected @ got @ <>
    if
        expected @ term.is_printable? '
        if
            expected @ hex expected !
        then

        got @ term.is_printable? '
        if
            got @ hex got !
        then

        "Did not get expected character, " expected @ + ", received " + got @ + "." + throw
    then
;


( Read numeric characters from the terminal until an expected terminator character is found. )
: term.read_num_until  ( terminator_char -- read_numberr )
    variable! until_char
    "" variable! read_str

    begin
        term.key

        dup until_char @ <>
        if
            read_str @ swap + read_str !
        then

        until_char @ =
    until

    read_str @ string.to_number
;


( Get the terminal's currrent cursor position. )
: term.get_cursor_position  ( -- row column )
    variable pos_r
    variable pos_c

    term.csi "6n" + term.! term.flush

    ( Expecting csi r ; c R )

    term.esc term.expect_key
    "[" term.expect_key
    ";" term.read_num_until pos_r !
    "R" term.read_num_until pos_c !

    pos_r @
    pos_c @
;


( Alternate ways to exit the interpreter. )
: q    ( -- ) quit ;
: exit ( -- ) quit ;


( Define a user prompt for the REPL. )
: prompt "\027[2:34m>\027[0:0m>" . ;


: repl.exit_handler "ok" .cr ;


( Implemntation of the language's REPL. )
: repl
    "Strange Forth REPL." .cr
    cr
    "Enter quit, q, or exit to quit the REPL." .cr
    cr

    at_exit repl.exit_handler

    begin
        ( Loop forever, because when the user enters the quit command the interpreter will exit )
        ( gracefully on it's own. )
        true
    while
        try
            ( Always make sure we get the newest version of the prompt.  That way the user can )
            ( change it at runtime. )
            cr "prompt" execute

            ( Get the text from the user and execute it.  We are just using a really simple )
            ( implementaion of readline for now. )
            term.readline
            code.execute_source

            ( If we got here, everything is fine. )
            "ok" .cr
        catch
            ( Something in the user code failed, display the error and try again. )
            .cr
        endcatch
    repeat
;


( Some user environment words. )
: user.home  ( -- home_path  ) "HOME"  user.env@ ;
: user.name  ( -- user_name  ) "USER"  user.env@ ;
: user.shell ( -- shell_path ) "SHELL" user.env@ ;
: user.term  ( -- term_name  ) "TERM"  user.env@ ;


( Quick hack to let scripts be executable from the command line. )
: #!/usr/bin/env ;
: sorth ;
