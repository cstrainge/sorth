
: variable immediate description: "Define a new variable."
    word op.def_variable
;


: @ immediate description: "Read from a variable index."
    op.read_variable
;


: ! immediate  description: "Write to a variable at the given index."
    op.write_variable
;


: variable! immediate description: "Define a new variable with a default value."
    word dup

    op.def_variable
    op.execute
    op.write_variable
;




: constant immediate description: "Define a new constant value."
    word op.def_constant
;




: sentinel_word hidden
    "This word should not be run directly outside of it's syntax." throw
;




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


: if immediate description: "Definition of the if else then syntax."
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

: else description: "Define an else clause for an if statement."
    sentinel_word
;

: then description: "End of an if/else/then block."
    sentinel_word
;




: break immediate description: "Break out of the current loop."
    op.jump_loop_exit
;


: continue immediate description: "Immediately jump to the next iteration of the loop."
    op.jump_loop_start
;




: begin immediate description: "Defines loop until and loop repeat syntaxes."
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

: until description: "The end of a loop/until block."
    sentinel_word
;

: while description: "Part of a begin/while/repeat block."
    sentinel_word
;

: repeat description: "The end of a begin/while/repeat block."
    sentinel_word
;




: ( immediate description: "Defines comment syntax."
    begin
        word ")" =
    until
;

: ) description: "The end of a comment block."
    sentinel_word
;




( Now that we've defined comments, we can begin to document the code.  So far we've defined a few  )
( base words.  Their implementations are to simply generate an instruction that will perform their )
( function into the bytecode stream being generated. )

( Next we define the 'if' statement.  The first one is just a basic version where else blocks are  )
( mandatory.  Right after that we redefine 'if' to have a more flexible implementation.  Note that )
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

: case immediate description: "Defines case/of/endcase syntax."
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
            ( beginning of the current code block. )
            true code.insert_at_front
            ` dup op.execute
            false code.insert_at_front

            ( Now, check to see if the value the case test left on the stack is equal to the input )
            ( we were given before the case statement began executing. )
            ` = op.execute

            ( If the test fails, jump to the next case test.  If it succeeds we drop the input )
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

: of description: "Defines a test clause of a case block."
    sentinel_word
;

: endof description: "Ends a clause of a case block."
    sentinel_word
;

: endcase description: "End of a case block."
    sentinel_word
;



( Simple increment and decrements. )
: ++ ( value -- incremented ) description: "Increment a value on the stack."
    1 +
;


: ++! ( variable -- ) description: "Increment the given variable."
    dup @ ++ swap !
;


: -- ( value -- decremented ) description: "Decrement a value on the stack."
    1 -
;


: --! ( variable -- ) description: "Decrement the given variable."
    dup @ -- swap !
;




( Make sure we have the regular printing words. )
: . ( value -- ) description: "Print a value with a space."
    term.!
    " " term.!
;


: .hex  ( value -- ) description: "Print a numeric value as hex."
    hex .
;


: cr ( -- ) description: "Print a newline to the console."
    "\n" term.!
    term.flush
;


: .cr  ( value -- ) description: "Print a value and a new line."
    . cr
;


: .hcr ( value -- ) description: "Print a hex value and a new line."
    .hex cr
;


: ? ( value -- ) description: "Print the value of a variable with a new line."
    @ .cr
;


: .sp ( count -- ) description: "Print the given number of spaces."
    begin
        -- dup 0 >=
    while
        " " term.!
    repeat
    drop
;




( Handy comparisons. )
: 0>  ( value -- test_result ) description: "Is the value greater than 0?"
    0 >
;


: 0=  ( value -- test_result ) description: "Does the value equal 0?"
    0 =
;


: 0<  ( value -- test_result ) description: "Is the value less than 0?"
    0 <
;


: 0>= ( value -- test_result ) description: "Is the value greater or equal to 0?"
    0 >=
;


: 0<= ( value -- test_result ) description: "Is the value less than or equal to 0?"
    0 <=
;




( Increment and decrement variables. )
: +! ( value variable -- ) description: "Increment a variable by a given value."
    over @ + swap !
;


: -! ( value variable -- ) description: "Decrement a variable by a given value."
    over @ - swap !
;




( String variable words. )
: string.length@ description: "Get the length of a string variable."
    ( string_var -- length )
    @ string.length
;


: string.find@ description: "Find the first instance of a sub-text within a string variable."
    ( search string_var -- position_or_npos )
    @ string.find
;


: string.to_number@ description: "Convert the string in a variable to a number."
    ( string_var -- new_number )
    @ string.to_number
;


: string.insert! description: "Insert a given sub-text into a string variable."
    ( sub_string position string_var -- )
    variable! var_index

    var_index @ @ string.insert
    var_index @ !
;


: string.remove! description: "Remove a count of characters from the given variable."
    ( count position string_var -- )
    variable! var_index

    var_index @ @ string.remove
    var_index @ !
;




( Quicker data field access. )
: #!! description: "Write to a structure field in a given variable."
    @ #!
;


: #@@ description: "Read from a structure field from a given variable."
    @ #@
;




( Array words. )
: []!! description: "Write a value at an index to the array variable."
    @ []!
;


: []@@ description: "Read a value from an index from the array variable."
    @ []@
;


: [].size@@ description: "Read the array variable's current size."
    @ [].size@
;


: [].size!! description: "Shrink or grow the array variable to the given size."
    @ [].size!
;




: [ immediate description: "Syntax definition for 'array [ index or indices ]' access."
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
                index_count @ index_blocks [].size!!
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

: , description: "Separator in the [ index , index , ... ] syntax."
    sentinel_word
;

: ]! description: "End of the [ index ] syntax.  Indicates an array write."
    sentinel_word
;

: ]!! description: "End of the [ index ] syntax.  Indicates a an array variable write."
    sentinel_word
;

: ]@ description: "End of the [ index ] syntax.  Indicates an array read."
    sentinel_word
;

: ]@@ description: "End of the [ index ] syntax.  Indicates an array variable read."
    sentinel_word
;



( Hash table words. )
: {}!! description: "Insert a value into the hash table variable."
    @ {}!
;


: {}@@ description: "Read a value from the hash table variable."
    @ {}@
;


: {}?? description: "Does a given key exist within the hash table variable?"
    @ {}?
;


: { immediate description: "Define the 'hash { key }' access syntax."
    variable command

    "}!" "}!!" "}@" "}@@" 4 code.compile_until_words

    case
        "}!"  of  ` {}!  command !  endof
        "}!!" of  ` {}!! command !  endof
        "}@"  of  ` {}@  command !  endof
        "}@@" of  ` {}@@ command !  endof
    endcase

    ` swap op.execute
    command @ op.execute
;

: }! description: "End of the { key } syntax.  Indicates a hash table write."
    sentinel_word
;

: }!! description: "End of the { key } syntax.  Indicates a a hash table variable write."
    sentinel_word
;

: }@ description: "End of the { key } syntax.  Indicates a hash table read."
    sentinel_word
;

: }@@ description: "End of the { key } syntax.  Indicates a hash table variable read."
    sentinel_word
;




( A try/catch block for exception handling. )
: try immediate description: "Define the try/catch/endcatch syntax."
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

: catch description: "End of the try block, starts the catch block."
    sentinel_word
;

: endcatch description: "End of the total try/catch/endcatch block."
    sentinel_word
;




( Helper words for reading/writing byte buffers. )
: buffer.i8!! description: "Write an 8-bit signed integer to the buffer variable."
    @ 1 buffer.int!
;


: buffer.i16!! description: "Write a 16-bit signed integer to the buffer variable."
    @ 2 buffer.int!
;


: buffer.i32!! description: "Write a 32-bit signed integer to the buffer variable."
    @ 4 buffer.int!
;


: buffer.i64!! description: "Write a  64-bit signed integer to the buffer variable."
    @ 8 buffer.int!
;



: buffer.i8@@ description: "Read an 8-bit signed integer from the buffer variable."
    @ 1 true buffer.int@
;


: buffer.i16@@ description: "Read a 16-bit signed integer from the buffer variable."
    @ 2 true buffer.int@
;


: buffer.i32@@ description: "Read a 32-bit signed integer from the buffer variable."
    @ 4 true buffer.int@
;


: buffer.i64@@ description: "Read a 64-bit signed integer from the buffer variable."
    @ 8 true buffer.int@
;



: buffer.u8@@ description: "Read an 8-bit unsigned integer from the buffer variable."
    @ 1 false buffer.int@
;


: buffer.u16@@ description: "Read a 16-bit unsigned integer from the buffer variable."
    @ 2 false buffer.int@
;


: buffer.u32@@ description: "Read a 32-bit unsigned integer from the buffer variable."
    @ 4 false buffer.int@
;


: buffer.u64@@ description: "Read a 64-bit unsigned integer from the buffer variable."
    @ 8 false buffer.int@
;



: buffer.f32!! description: "Write a 32-bit floating point value to the buffer variable."
    @ 4 buffer.float!
;


: buffer.f64!! description: "Write a 64-bit floating point value to the buffer variable."
    @ 8 buffer.float!
;



: buffer.f32@@ description: "Read a 32-bit floating point value from the buffer variable."
    @ 4 buffer.float@
;


: buffer.f64@@ description: "Read a 64-bit floating point value from the buffer variable."
    @ 8 buffer.float@
;



: buffer.string!!
    description: "Write a string of a given size to the buffer variable.  Pad with 0s."
    @ swap buffer.string!
;


: buffer.string@@ description: "Read a string of max size from the buffer variable."
    @ swap buffer.string@
;



: buffer.position!! description: "Set the current buffer pointer to the buffer in variable."
    @ buffer.position!
;


: buffer.position@@ description: "Read the current buffer pointer from the variable."
    @ buffer.position@
;




( Allow user code to register an at exit handler. )
: at_exit immediate description: "Request that a given word be executed when the script exits."
    word op.push_constant_value
    ` at_exit op.execute
;




( Check for extra terminal functionality.  If it's there include some extra useful words. )
defined? term.raw_mode
if
    "std/term.f" include
then




( If we have the user environment available, include some more useful words. )
defined? user.env@
if
    "std/user.f" include
then




( Alternate ways to exit the interpreter. )
: q    ( -- ) description: "Exit the interpreter."
    quit
;


: exit ( -- ) description: "Exit the interpreter."
    quit
;




( Make sure that advanced terminal and user functionality is available.  If it is, enable the )
( 'fancy' repl capable of keeping history.  Otherwise enable the simpler repl. )
defined? term.raw_mode
defined? user.env@
&&
if
    "std/repl.f" include
else
    "std/simple_repl.f" include
then




( Include our json utility functions. )
"std/json.f" include




( Quick hack to let scripts be executable from the command line. )
: #!/usr/bin/env hidden ;
: sorth hidden ;
