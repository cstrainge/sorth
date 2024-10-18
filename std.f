
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
    "The word " swap + " should not be run directly outside of it's syntax." + throw
;




: mark_context immediate description: "Create a new variable and word context."
                         signature: " -- "
    op.mark_context
;


: release_context immediate description: "Release the current context freeing it's variables and words."
                         signature: " -- "
    op.release_context
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
    "else" sentinel_word
;

: then immediate description: "End of an if/else/then block."
    "then" sentinel_word
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
    "until" sentinel_word
;

: while description: "Part of a begin/while/repeat block."
    "while" sentinel_word
;

: repeat description: "The end of a begin/while/repeat block."
    "repeate" sentinel_word
;




: ( immediate description: "Defines comment syntax."
    begin
        word ")" =
    until
;

: ) immediate description: "The end of a comment block."
    ")" sentinel_word
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
    "of" sentinel_word
;

: endof immediate description: "Ends a clause of a case block."
    "endof" sentinel_word
;

: endcase immediate description: "End of a case block."
    "endcase" sentinel_word
;



( Simple increment and decrements. )
: ++  description: "Increment a value on the stack."
      signature: "value -- incremented"
    1 +
;


: ++!  description: "Increment the given variable."
       signature: "variable -- "
    dup @ ++ swap !
;


: --  description: "Decrement a value on the stack."
      signature: "value -- decremented"
    1 -
;


: --!  description: "Decrement the given variable."
       signature: "variable -- "
    dup @ -- swap !
;




: do immediate description: "Define a do loop syntax."
               signature: "start_value end_value do <loop-body> loop"
    ( Keep track of the start and end labels for the loop. )
    unique_str variable! top_label
    unique_str variable! end_label

    ( Create a new sub-block of instructions for this loop. )
    code.new_block

    ( Create variables to track the end boundary and loop index. )
    unique_str variable! end_value
    unique_str variable! index


    ( Get the end value off the top of the stack and store in a constant it for comparison. )
    end_value @  op.def_constant


    ( Define the loop index starting at the next value on the stack. )
    index @ dup  op.def_variable
                 op.execute
    op.write_variable

    ( Mark the beginning of the loop. )
    end_label @ op.mark_loop_exit
    top_label @ op.jump_target

    ( Generate the loop comparison. )
    index @ op.execute
    op.read_variable

    end_value @ op.execute

    ` < op.execute
    end_label @ op.jump_if_zero


    ( Compile the loop body. )
    "loop" 1 code.compile_until_words
    drop


    ( Compile the increment and loop repeat. )
    index @ op.execute
    ` ++! op.execute
    top_label @ op.jump

    ( Mark the end of the loop. )
    end_label @ op.jump_target
    op.unmark_loop_exit


    ( Clean up and merge the new code. )
    code.resolve_jumps
    code.merge_stack_block
;


: loop description: "The end of a do loop."
    "loop" sentinel_word
;




( Stack words. )
: nip description: "Nip the second from the top item from the stack."
      signature: "a b c -- a c"
    swap
    drop
;



: 2drop description: "Drop the top two items from the stack."
        signature: "a b -- "
    drop
    drop
;



: 3drop description: "Drop the top three items from the stack."
        signature: "a b -- "
    drop
    drop
    drop
;



: 4drop description: "Drop the top two items from the stack."
        signature: "a b -- "
    drop
    drop
    drop
    drop
;



: clear description: "Clear out the stack."
        signature: " -- "
    begin
        stack.depth  0  <>
    while
        drop
    repeat
;




( Make sure we have the regular printing words. )
: .  description: "Print a value with a space."
     signature: "value -- "
    term.!
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


: .sp  description: "Print the given number of spaces."
       signature: "count -- "
    begin
        -- dup 0 >=
    while
        " " term.!
    repeat
    drop
;



: thread.new immediate description: "Create a new thread with the given word and return the new thread id."
             signature: "thread.new <word_name>"
    word op.push_constant_value
    ` thread.new op.execute
;



: value.both-are? description: "Check if the two values are the same type."
                  signature: "a b value-check -- are-same-type?"
    variable! operation
    variable! b
    variable! a

    a @  operation @  execute
    b @  operation @  execute
    &&
;



: value.both-are-hash-tables? description: "Are two values hash tables?"
                              signature: "a b -- are-hash-tables?"
    ` value.is-hash-table?  value.both-are?
;



: value.both-are-arrays? description: "Are two values arrays?"
                         signature: "a b -- are-arrays?"
    ` value.is-array?  value.both-are?
;



: value.both-are-structures? description: "Are two values structures?"
                             signature: "a b -- are-structures?"
    ` value.is-structure?  value.both-are?
;



: value.both-are-strings? description: "Are two values strings?"
                          signature: "a b -- are-strings?"
    ` value.is-string?  value.both-are?
;



: value.both-are-numbers? description: "Are two values numbers?"
                          signature: "a b -- are-numbers?"
    ` value.is-number?  value.both-are?
;



: value.both-are-booleans? description: "Are two values booleans?"
                           signature: "a b -- are-boolean?"
    ` value.is-boolean?  value.both-are?
;



: = description: "Compare two values."
    signature: "a b -- are_equal?"
    variable! b
    variable! a

    a @ b @  value.both-are-structures?
    if
        a @ b @  #.=
    else
        a @ b @  value.both-are-hash-tables?
        if
            a @ b @  {}.=
        else
            a @ b @  value.both-are-arrays?
            if
                a @ b @  [].=
            else
                a @ b @  =
            then
        then
    then
;



: <> description: "Compare two values."
     signature: "a b -- are-not-equal?"
    = '
;



: + description: "Add two values together."
    signature: "a b -- result"
    variable! b
    variable! a

    a @ b @  value.both-are-hash-tables?
    if
        a @ b @  {}.+
    else
        a @ b @  value.both-are-arrays?
        if
            a @ b @  [].+
        else
            a @ b @  +
        then
    then
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


: string.[]@@ description: "Read a character from a given string variable."
              signature: "index variable -- character"
    @ string.[]@
;


: string.remove! description: "Remove a count of characters from the given variable."
                 signature: "count position string_variable -- updated_string"
    variable! var_index

    var_index @ @ string.remove
    var_index @ !
;


: string.substring description: "Extract a substring from an existing string."
                   signature: "start end string -- substring"
    variable! string

    variable! end_index
    variable! start_index

    end_index @ string.npos =
    if
        string @ string.size@ -- end_index !
    then

    start_index @ variable! index

    "" variable! sub_string

    start_index @  string @ string.size@  >=
    end_index @    string @ string.size@  >=
    ||
    if
        start_index @
        end_index @
        string @ string.size@
        "Substring indices are out of , ({}, {} / {},) bounds."
        string.format throw
    then

    start_index @  end_index @  >
    if
        start_index @  end_index @ "Start and end index, ({}, {},) in reverse order."
        string.format throw
    then

    begin
        index @  end_index @  <=
    while
        sub_string @  index @ string @ string.[]@  +  sub_string !
        index ++!
    repeat

    sub_string @
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


: [].size++!  description: "Grow an array by one item."
              signature: "array -- "
    variable! the_array

    the_array [].size@@ ++ the_array [].size!!
;


: [].size++!!  description: "Grow an array variable by one item."
               signature: "array_variable -- "
    @ variable! the_array

    the_array [].size@@ ++ the_array [].size!!
;


: [].size--!!  description: "Shrink an array variable by one item."
               signature: "array_variable -- "
    @ variable! the_array

    the_array [].size@@ -- the_array [].size!!
;


: [].push_front!! description: "Push a new value to the top of an array variable."
                  signature: "value array_variable -- "
    @ [].push_front!
;

: [].push_back!! description: "Push a new value to the end of an array variable."
                  signature: "value array_variable -- "
    @ [].push_back!
;

: [].pop_front!! description: "Pop a value from the top of an array variable."
                  signature: "array_variable -- value"
    @ [].pop_front!
;

: [].pop_back!! description: "Pop a value from the bottom of an array variable."
                  signature: "array_variable -- value"
    @ [].pop_back!
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
    "," sentinel_word
;

: ]! immediate description: "End of the [ index ] syntax.  Indicates an array write."
    "]!" sentinel_word
;

: ]!! immediate description: "End of the [ index ] syntax.  Indicates a an array variable write."
    "]!!" sentinel_word
;

: ]@ immediate description: "End of the [ index ] syntax.  Indicates an array read."
    "]@" sentinel_word
;

: ]@@ immediate description: "End of the [ index ] syntax.  Indicates an array variable read."
    "]@@" sentinel_word
;




[defined?] clr.to-string
[if]
    : to_string description: "Convert a value to a string."
                signature: "value -- string"

        ( Check if this is a clr object, if it is use the CLR's built-in string formatting. )
        dup value.is-clr-object?
        if
            clr.to-string
        else
            ( Looks like a native value, use Forth's string formatting. )
            to_string
        then
    ;
[then]




: string.split description: "Given a split character, split a string into an array of strings."
               signature: "split_char string -- string_array"
    variable! string
    constant splitter

    string @ string.size@ constant string_size

    [ "" ] variable! output
    0 variable! output_index

    0 variable! index
    variable next

    begin
        index @  string_size  <
    while
        index @ string string.[]@@ next !

        splitter  next @  =
        if
            output [].size++!!
            output_index ++!
            "" output [ output_index @ ]!!
        else
            output [ output_index @ ]@@ next @ +  output [ output_index @ ]!!
        then

        index ++!
    repeat

    output [ output_index @ ]@@  string.size@  0=
    if
        output [].size--!!
    then

    output @
;




( Given a format string and a position after the beginning bracket { extract the substring found )
( within those brackets, {}  If there no specifier there an empty string, "", is returned instead. )
( in both cases an updated index is also returned that points to after the closing bracket, }. )
: string.format.extract_specifier  hidden  ( index format_string -- new_index specifier_string )
    variable! format_str
    variable! char_index
    format_str @ string.size@ variable! length

    "" variable! specifier
    variable next

    char_index @   format_str @  string.[]@  "}" <>
    if
        begin
            char_index @  length @  <
        while
            char_index @  format_str @  string.[]@  next !

            next @  "}"  <>
            if
                specifier @  next @  +  specifier !
            else
                break
            then

            char_index ++!
        repeat

        next @  "}"  <>
        if
            "Missing closing } in format specifier." throw
        then
    then

    char_index @
    specifier @
;


( Get a character from the string at the given index.  If the index is outside of the string an )
( empty string is returned instead. )
: string.format.get_char  hidden  ( index string -- character )
    variable! string
    variable! index

    string @ string.size@ variable! size

    "" variable! char

    index @  size @  <
    if
        index @  string string.[]@@  char !
    then

    char @
;


( Parse the specifier found in the format string and return it's values broken out.  If the string )
( is empty or a component is missing then the default is returned in it's stead. )
: string.format.parse_specifier  hidden  ( value specifier -- fill alignment width is_hex )
    variable! specifier
    variable! value

    " " variable! fill
    value @ value.is-number? if ">" else "<" then variable! alignment
    ""  variable! width
    false variable! is_hex

    variable char
    0 variable! index

    specifier string.size@@ variable! size

    specifier @  ""  <>
    if
        index @ specifier @ string.format.get_char char !

        char @  "<"  =  char @  "^"  =  ||  char @  ">"  =  ||
        if
            char @  alignment !
            index ++!
        then

        index @ specifier @ string.format.get_char char !

        char @  "0"  <  char @  "9"  >  ||
        char @  "0"  =
        ||
        if
            char @ fill !
            index ++!
        then

        index @ specifier @ string.format.get_char char !

        begin
            char @  "0"  >=  char @  "9"  <=  &&
            index @  size @  <
            &&
        while
            width @ char @ +  width !

            index ++!
            index @ specifier @ string.format.get_char char !
        repeat

        width @ string.size@ 0>
        if
            width @ string.to_number width !
        else
            0 width !
        then

        index @ specifier @ string.format.get_char char !

        char @  "x"  =
        char @  "X"  =
        ||
        is_hex !
    else
        0 width !
    then

    fill @
    alignment @
    width @
    is_hex @
;


( Given a count and a character create a string of count width filled with that character. )
: string.format.fill_str  hidden  ( count char -- fill_string )
    variable! char
    variable! count

    0 variable! index

    "" variable! new_str

    begin
        index @  count @  <
    while
        new_str @  char @  +  new_str !
        index ++!
    repeat

    new_str @
;


( Given a value and a sub-specifier convert the value to a string and format it according to the )
( specifier string. )
: string.format_value  hidden  ( value specifier -- formatted_value )
    variable! specifier
    variable! value

    variable fill
    variable alignment
    variable width
    variable is_hex

    0 variable! fill_width

    variable str_value

    specifier @  ""  <>
    if
        value @ specifier @ string.format.parse_specifier is_hex ! width ! alignment ! fill !

        is_hex @
        if
            value @ value.is-number?
            if
                value @ hex str_value !
            else
                "Can't convert value to a hex string." throw
            then
        else
            value @ to_string str_value !
        then

        str_value string.size@@  width  <
        if
            width @ str_value string.size@@ -  fill_width !

            alignment @
            case
                "<" of
                        str_value @  fill_width @ fill @ string.format.fill_str  +  str_value !
                    endof

                "^" of
                        fill_width @ 2 / fill @ string.format.fill_str  str_value @  +  str_value !

                        fill_width @ 2 %  0  <>
                        if
                            fill_width @ 2 / 1 + fill_width !
                        else
                            fill_width @ 2 / fill_width !
                        then

                        str_value @  fill_width @ fill @ string.format.fill_str  +  str_value !
                    endof

                ">" of
                        fill_width @ fill @ string.format.fill_str  str_value @  +  str_value !
                    endof
            endcase
        then
    else
        value @ to_string str_value !
    then

    str_value @
;


: string.format
    description: "Format a string where occurrences of {} are replaced with stack values."
    signature: "[variables] format_string -- formatted_string"

    0 [].new variable! snippets
    0 [].new variable! values
    0 [].new variable! specifiers

    variable! format_str

    format_str string.size@@ variable! length
    0 variable! char_index
    "" variable! format_snippet

    variable next

    begin
        char_index @  length @  <
    while
        char_index @  format_str @  string.[]@  next !

        next @  "{"  =
        if
            char_index @ ++ format_str @ string.format.extract_specifier
            specifiers [].push_back!!
            char_index !

            format_snippet @  snippets  [].push_back!!
                                values  [].push_front!!

            "" format_snippet !
        else
            format_snippet @  next @  +  format_snippet !
        then

        char_index ++!
    repeat

    0 variable! snippet_index
    "" variable! output_string

    begin
        snippet_index @  snippets [].size@@  <
    while
        output_string @  snippets [ snippet_index @ ]@@  +  output_string !


        snippet_index @  values [].size@@  <
        if
            output_string @
            values [ snippet_index @ ]@@  specifiers [ snippet_index @ ]@@  string.format_value
            +

            output_string !
        then

        snippet_index ++!
    repeat

    format_snippet string.size@@  0>
    if
        output_string @ format_snippet @  +  output_string !
    then

    output_string @
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
    "}!" sentinel_word
;

: }!! immediate description: "End of the { key } syntax.  Indicates a a hash table variable write."
    "}!!" sentinel_word
;

: }@ immediate description: "End of the { key } syntax.  Indicates a hash table read."
    "}@" sentinel_word
;

: }@@ immediate description: "End of the { key } syntax.  Indicates a hash table variable read."
    "}@@" sentinel_word
;

: } immediate description: "Hash table definition syntax."
    "}" sentinel_word
;



: # immediate description: "Beginning of a structure definition."
              signature: "# name field_name [ -> default_value ] ... ;"
    word variable! struct_name
    false variable! is_hidden
    variable field_name

    0 [].new variable! fields
    0 [].new variable! defaults
    0 variable! index

    false variable! found_initializers

    code.new_block

    begin
        true
    while
        word field_name !

        field_name @
        case
            "hidden" of
                    true is_hidden !
                    continue
                endof

            "(" of
                    "(" execute
                    continue
                endof

            "->" of
                    true found_initializers !

                    ` dup op.execute
                    ";" "," 2 code.compile_until_words

                    ` swap op.execute
                    index @ -- op.push_constant_value
                    ` swap op.execute
                    ` []! op.execute

                    ";" =
                    if
                        break
                    then
                endof

            ";" of
                    break
                endof

            index @ ++ fields [].size!!
            index @ ++ defaults [].size!!

            field_name @ fields [ index @ ]!!

            index ++!
        endcase
    repeat

    found_initializers @
    if
        true code.insert_at_front
        defaults @ op.push_constant_value
        false code.insert_at_front
    then

    struct_name @        op.push_constant_value
    fields @             op.push_constant_value
    is_hidden @          op.push_constant_value
    found_initializers @ op.push_constant_value

    ` # op.execute

    code.merge_stack_block
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
    "catch" sentinel_word
;

: endcatch immediate description: "End of the total try/catch/endcatch block."
    "endcatch" sentinel_word
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




( Given an array and an operator go through the array and select out one of the values using that )
( operator. )
: one_of hidden  ( array operator -- chosen-value )
    variable! operator
    variable! values

    values [].size@@ constant size

    size  0<=
    if
        "No values in array." throw
    then

    values [ 0 ]@@ variable! chosen
    1 variable! index

    begin
        index @  size  <
    while
        values [ index @ ]@@ dup  chosen @  operator @ execute
        if
            chosen !
        else
            drop
        then

        index ++!
    repeat

    chosen @
;




: min_of description: "Get the minimum of an array of values."
         signature: "array -- smallest-value"
    ` < one_of
;




: max_of description: "Get the maximum of an array of values."
         signature: "array -- smallest-value"
    ` > one_of
;




: min description: "Get the minimum of two values."
      signature: "a b -- [a or b]"
    variable! b
    variable! a

    [ a @ , b @ ]  ` <  one_of
;




: max description: "Get the maximum of two values."
      signature: "a b -- [a or b]"
    variable! b
    variable! a

    [ a @ , b @ ]  ` >  one_of
;




: [&&] immediate  description: "Evaluate && at compile time."
                  signature: "a b -- result"
    &&
;




: [||] immediate  description: "Evaluate || at compile time."
                  signature: "a b -- result"
    ||
;




( Check for extra terminal functionality.  If it's there include some extra useful words. )
[defined?] term.raw_mode
[if]
    [include] std/term.f
[then]




( If we have the user environment available, include some more useful words. )
: [is-windows?] immediate description: "Evaluate at compile time, is the OS Windows?"
                          signature: " -- bool"
    user.os  "Windows"  =
;

: [is-macos?] immediate description: "Evaluate at compile time, is the OS macOS?"
                        signature: " -- bool"
    user.os  "macOS"  =
;

: [is-linux?] immediate description: "Evaluate at compile time, is the OS Linux?"
                        signature: " -- bool"
    user.os  "Linux"  =
;

[defined?] user.env@
[if]
    [is-windows?]
    [if]
        [include] std/win_user.f
    [else]
        [include] std/user.f
    [then]
[then]




( Make sure that advanced terminal and user functionality is available.  If it is, enable the )
( 'fancy' repl capable of keeping history.  Otherwise enable the simpler repl. )
[defined?] term.raw_mode
[defined?] user.env@
[&&]
[if]
    [include] std/repl.f
[else]
    [include] std/simple_repl.f
[then]




[undefined?] show_word
[if]
    : show_word
        variable! name

        words.get{} { name @ }@

               dup sorth.word.handler_index@
          swap dup sorth.word.name@
          swap dup sorth.word.location@
                   dup sorth.location.path@
              swap dup sorth.location.line@
                  swap sorth.location.column@
        3 pick dup sorth.word.description@
          swap dup sorth.word.signature@
        swap 7 push-to
        "*Word:        {} -> {}
          Defined:     {}:{}:{}

          Description: {}
          Signature:   {}*"
        string.format .cr

        dup sorth.word.is_immediate@
        if
            "\n             The word is immediate." .
        then

        sorth.word.is_scripted@
        if
            "\n             The word is written in Forth." .
        else
            "\n             The word is a native word." .
        then

        cr
    ;
[then]




: show_word immediate description: "Show detailed information about a word."
            signature: "show_word <word_name>"
    word op.push_constant_value
    ` show_word op.execute
;




( Include our json utility functions. )
[include] std/json.f




( Include the ffi system. )
[include] std/ffi.f




( Quick hack to let scripts be executable from the command line. )
: #!/usr/bin/env hidden ;
: sorth hidden ;
