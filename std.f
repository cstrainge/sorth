
: variable immediate description: "Define a new variable."
                     signature: "variable <new_name>"
    word op.def_variable
;


: @ immediate description: "Read from a variable index."
              signature: "variable -- value"
    op.read_variable
;


: ! immediate  description: "Write to a variable at the given index."
               signature: "value variable -- "
    op.write_variable
;


: variable! immediate description: "Define a new variable with a default value."
                      signature: "new_value variable! <new_name>"
    word dup

    op.def_variable
    op.execute
    op.write_variable
;




: constant immediate description: "Define a new constant value."
                     signature: "new_value constant <new_name>"
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
               signature: "<test> if <code> [else <code>] then"
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

: else immediate description: "Define an else clause for an if statement."
    sentinel_word
;

: then immediate description: "End of an if/else/then block."
    sentinel_word
;




: break immediate description: "Break out of the current loop."
    op.jump_loop_exit
;


: continue immediate description: "Immediately jump to the next iteration of the loop."
    op.jump_loop_start
;




: begin immediate description: "Defines loop until and loop repeat syntaxes."
                  signature: "begin <code> <test> until *or* begin <test> while <code> repeat"
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

: ) immediate description: "The end of a comment block."
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
                 signature: "<value> case <value> of <code> endof ... <code> endcase"
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

: of immediate description: "Defines a test clause of a case block."
    sentinel_word
;

: endof immediate description: "Ends a clause of a case block."
    sentinel_word
;

: endcase immediate description: "End of a case block."
    sentinel_word
;



( Simple increment and decrements. )
: ++  description: "Increment a value on the stack."
      ( value -- incremented )
    1 +
;


: ++!  description: "Increment the given variable."
       ( variable -- )
    dup @ ++ swap !
;


: --  description: "Decrement a value on the stack."
      ( value -- decremented )
    1 -
;


: --!  description: "Decrement the given variable."
       ( variable -- )
    dup @ -- swap !
;




( Make sure we have the regular printing words. )
: .  description: "Print a value with a space."
     signature: "value -- "
    term.!
    " " term.!
;


: .hex  description: "Print a numeric value as hex."
        signature: "value -- "
    hex .
;


: cr  description: "Print a newline to the console."
      signature: " -- "
    "\n" term.!
    term.flush
;


: .cr  description: "Print a value and a new line."
       signature: "value -- "
    . cr
;


: .hcr  description: "Print a hex value and a new line."
        signature: "value -- "
    .hex cr
;


: ?  description: "Print the value of a variable with a new line."
     signature: "value -- "
    @ .cr
;


: .sp  description: "Print the given number of spaces."
       signature: "count -- "
    begin
        -- dup 0 >=
    while
        " " term.!
    repeat
    drop
;


: show_word immediate description: "Show detailed information about a word."
            signature: "show_word <word_name>"
    word op.push_constant_value
    ` show_word op.execute
;




( Handy comparisons. )
: 0>  description: "Is the value greater than 0?"
      signature: "value -- test_result"
    0 >
;


: 0=  description: "Does the value equal 0?"
      signature: "value -- test_result"
    0 =
;


: 0<  description: "Is the value less than 0?"
      signature: "value -- test_result"
    0 <
;


: 0>=  description: "Is the value greater or equal to 0?"
       signature: "value -- test_result"
    0 >=
;


: 0<=  description: "Is the value less than or equal to 0?"
       signature: "value -- test_result"
    0 <=
;




( Increment and decrement variables. )
: ++!  description: "Increment a variable by a given value."
       signature: "value variable -- "
     dup @ ++ swap !
;


: --!  description: "Decrement a variable by a given value."
       signature: "value variable -- "
    dup @ -- swap !
;




( String variable words. )
: string.size@@ description: "Get the length of a string variable."
                signature: "string_variable -- length"
    @ string.size@
;


: string.find@ description: "Find the first instance of a sub-text within a string variable."
               signature: "search string_variable -- position_or_npos"
    @ string.find
;


: string.to_number@ description: "Convert the string in a variable to a number."
                    signature: " string_variable -- new_number "
    @ string.to_number
;


: string.[]!! description: "Insert a given sub-text into a string variable."
              signature: "sub_string position string_variable -- updated_string"
    variable! var_index

    var_index @ @ string.[]!
    var_index @ !
;


: string.remove! description: "Remove a count of characters from the given variable."
                 signature: "count position string_variable -- updated_string"
    variable! var_index

    var_index @ @ string.remove
    var_index @ !
;




( Quicker data field access. )
: #!! description: "Write to a structure field in a given variable."
      signature: "value field_index struct_variable -- "
    @ #!
;


: #@@ description: "Read from a structure field from a given variable."
      signature: "field_index struct_variable -- value"
    @ #@
;




( Array words. )
: []!! description: "Write a value at an index to the array variable."
       signature: "new_value index array_variable -- "
    @ []!
;


: []@@ description: "Read a value from an index from the array variable."
       signature: "index array_variable -- value"
    @ []@
;


: [].size@@ description: "Read the array variable's current size."
            signature: "array_variable -- size"
    @ [].size@
;


: [].size!! description: "Shrink or grow the array variable to the given size."
            signature: "new_size array_variable -- "
    @ [].size!
;


: [].size++!  description: "Grow an array by one item, leaving the resized array on the stack."
              signature: "array -- resized_array"
    variable! the_array

    the_array [].size@@ ++ the_array [].size!!
    the_array @
;


: [].size++!!  description: "Grow an array variable by one item."
               signature: "array_variable -- "
    @ variable! the_array

    the_array [].size@@ ++ the_array [].size!!
;




: [ immediate
    description: "Define 'array [ index or indices ]' access or `[ value , ... ]` creation."
    signature: "array [ <index> ]<operation> *or* [ <value_list> ]"

    1 variable! index_count
    1 [].new variable! index_blocks

    variable command

    false variable! found_end_bracket
    true variable! is_writing
    false variable! is_creating

    code.new_block

    begin
        "," "]!" "]!!" "]@" "]@@" "]" 6 code.compile_until_words

        code.pop_stack_block index_count @ -- index_blocks []!!

        case
            "," of
                index_count ++!

                code.new_block
                index_count @ index_blocks [].size!!
            endof

            "]"   of  ` [].new command !  true found_end_bracket !  true is_creating !  endof
            "]!"  of  ` []!  command !    true found_end_bracket !                      endof
            "]!!" of  ` []!! command !    true found_end_bracket !                      endof
            "]@"  of  ` []@  command !    true found_end_bracket !  false is_writing !  endof
            "]@@" of  ` []@@ command !    true found_end_bracket !  false is_writing !  endof
        endcase

        found_end_bracket @
    until

    is_creating @
    if
        index_count @ op.push_constant_value
        command @ op.execute

        0 index_count !

        begin
            index_count @ index_blocks [].size@@ <
        while
            index_count @ index_blocks []@@ code.push_stack_block

            code.stack_block_size@ 0>
            if
                true code.insert_at_front
                ` dup op.execute
                false code.insert_at_front

                ` swap op.execute
                index_count @ op.push_constant_value
                ` swap op.execute
                ` []! op.execute
            then

            code.merge_stack_block
            index_count ++!
        repeat
    else
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
    then
;

: , immediate description: "Separator in the [ index , ... ] and { key -> value , ... } syntaxes."
    sentinel_word
;

: ]! immediate description: "End of the [ index ] syntax.  Indicates an array write."
    sentinel_word
;

: ]!! immediate description: "End of the [ index ] syntax.  Indicates a an array variable write."
    sentinel_word
;

: ]@ immediate description: "End of the [ index ] syntax.  Indicates an array read."
    sentinel_word
;

: ]@@ immediate description: "End of the [ index ] syntax.  Indicates an array variable read."
    sentinel_word
;



( Hash table words. )
: {}!! description: "Insert a value into the hash table variable."
       signature: "value key hash_variable -- "
    @ {}!
;


: {}@@ description: "Read a value from the hash table variable."
       signature: "key hash_variable -- value"
    @ {}@
;


: {}?? description: "Does a given key exist within the hash table variable?"
       signature: "key hash_variable -- does_exist?"
    @ {}?
;


: { immediate description: "Define both the 'hash { key }'  and '{ key -> value , ... }' syntaxes."
              signature: "hash { key }<operation> *or* { key -> value , ... }"
    variable command

    false variable! is_new
    false variable! is_inline_syntax

    "->" "}" "}!" "}!!" "}@" "}@@" 6 code.compile_until_words

    case
        "->"  of  true is_inline_syntax !                 endof
        "}"   of  ( "Missing word -> and key value." throw )  endof
        "}!"  of  ` {}!  command !                        endof
        "}!!" of  ` {}!! command !                        endof
        "}@"  of  ` {}@  command !                        endof
        "}@@" of  ` {}@@ command !                        endof
    endcase

    false is_inline_syntax @ =
    if
        ` swap op.execute
        command @ op.execute
    else
        ` {}.new op.execute
        ` over op.execute
        "," "}" 2 code.compile_until_words
        ` rot op.execute
        ` {}! op.execute

        ` dup op.execute

        "}" <>
        if
            begin
                "->" "}" 2 code.compile_until_words
                "->" =
            while
                ` swap op.execute
                "," "}" 2 code.compile_until_words
                ` rot op.execute
                ` {}! op.execute
                ` dup op.execute

                "}" =
                if
                    break
                then
            repeat
        then

        ` drop op.execute
    then
;

: }! immediate description: "End of the { key } syntax.  Indicates a hash table write."
    sentinel_word
;

: }!! immediate description: "End of the { key } syntax.  Indicates a a hash table variable write."
    sentinel_word
;

: }@ immediate description: "End of the { key } syntax.  Indicates a hash table read."
    sentinel_word
;

: }@@ immediate description: "End of the { key } syntax.  Indicates a hash table variable read."
    sentinel_word
;

: } immediate description: "Hash table definition syntax."
    sentinel_word
;



: #.new immediate description: "Create a new instance of the named structure."
                  signature: "#.new struct_name { field -> value , ... }"
    word variable! struct_name

    word dup "{" <>
    if
        "Expected { word to open structure creation, found " swap + "." + throw
    then
    drop

    struct_name @ ".new" + op.execute

    variable next_word
    variable field_name

    begin
        next_word @ "}" <>
    while
        word field_name !

        word dup "->" <>
        if
            "Expected assignment operator, ->, found " swap + "." + throw
        then
        drop

        "," "}" 2 code.compile_until_words
        next_word !

        ` swap op.execute
        ` over op.execute
        struct_name @ "." + field_name @ + op.execute
        ` swap op.execute
        ` #! op.execute
    repeat
;




( A try/catch block for exception handling. )
: try immediate description: "Define the try/catch/endcatch syntax."
                signature: "try <code> catch <code> endcatch"
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

: catch immediate description: "End of the try block, starts the catch block."
    sentinel_word
;

: endcatch immediate description: "End of the total try/catch/endcatch block."
    sentinel_word
;




( Helper words for reading/writing byte buffers. )
: buffer.i8!! description: "Write an 8-bit signed integer to the buffer variable."
              signature: "value buffer_variable -- "
    @ 1 buffer.int!
;


: buffer.i16!! description: "Write a 16-bit signed integer to the buffer variable."
               signature: "value buffer_variable -- "
    @ 2 buffer.int!
;


: buffer.i32!! description: "Write a 32-bit signed integer to the buffer variable."
               signature: "value buffer_variable -- "
    @ 4 buffer.int!
;


: buffer.i64!! description: "Write a 64-bit signed integer to the buffer variable."
               signature: "value buffer_variable -- "
    @ 8 buffer.int!
;



: buffer.i8@@ description: "Read an 8-bit signed integer from the buffer variable."
              signature: "buffer_variable -- value"
    @ 1 true buffer.int@
;


: buffer.i16@@ description: "Read a 16-bit signed integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 2 true buffer.int@
;


: buffer.i32@@ description: "Read a 32-bit signed integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 4 true buffer.int@
;


: buffer.i64@@ description: "Read a 64-bit signed integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 8 true buffer.int@
;



: buffer.u8@@ description: "Read an 8-bit unsigned integer from the buffer variable."
              signature: "buffer_variable -- value"
    @ 1 false buffer.int@
;


: buffer.u16@@ description: "Read a 16-bit unsigned integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 2 false buffer.int@
;


: buffer.u32@@ description: "Read a 32-bit unsigned integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 4 false buffer.int@
;


: buffer.u64@@ description: "Read a 64-bit unsigned integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 8 false buffer.int@
;



: buffer.f32!! description: "Write a 32-bit floating point value to the buffer variable."
               signature: "buffer_variable -- value"
    @ 4 buffer.float!
;


: buffer.f64!! description: "Write a 64-bit floating point value to the buffer variable."
               signature: "buffer_variable -- value"
    @ 8 buffer.float!
;



: buffer.f32@@ description: "Read a 32-bit floating point value from the buffer variable."
               signature: "buffer_variable -- value"
    @ 4 buffer.float@
;


: buffer.f64@@ description: "Read a 64-bit floating point value from the buffer variable."
               signature: "buffer_variable -- value"
    @ 8 buffer.float@
;



: buffer.string!!
    description: "Write a string of a given size to the buffer variable.  Pad with 0s."
    signature: "string buffer_variable max_size -- "
    @ swap buffer.string!
;


: buffer.string@@ description: "Read a string of max size from the buffer variable."
                  signature: "buffer_variable max_size -- string"
    @ swap buffer.string@
;



: buffer.position!! description: "Set the current buffer pointer to the buffer in variable."
                    signature: "new_position buffer_variable -- "
    @ buffer.position!
;


: buffer.position@@ description: "Read the current buffer pointer from the variable."
                    signature: "buffer_variable -- position"
    @ buffer.position@
;




( Allow user code to register an at exit handler. )
: at_exit immediate description: "Request that a given word be executed when the script exits."
                    signature: "at_exit <word>"
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
: q  description: "Exit the interpreter."
     signature: "[exit_code] -- "
    quit
;


: exit description: "Exit the interpreter."
       signature: "[exit_code] -- "
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
