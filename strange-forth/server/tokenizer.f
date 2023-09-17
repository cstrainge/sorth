
( Represent a given location within the source text. )
# tk.location
    uri        ( The uri we get from vscode for the file being edited. )
    line       ( Zero based line that we are on. )
    character  ( Zero based column that we are on. )
;




: tk.location.uri!        tk.location.uri        swap @ #! ;
: tk.location.line!       tk.location.line       swap @ #! ;
: tk.location.character!  tk.location.character  swap @ #! ;

: tk.location.uri@        tk.location.uri        swap @ #@ ;
: tk.location.line@       tk.location.line       swap @ #@ ;
: tk.location.character@  tk.location.character  swap @ #@ ;




: tk.location.new  ( uri -- tk.location )
    tk.location.new variable! location

      location tk.location.uri!
    0 location tk.location.line!
    0 location tk.location.character!

    location @
;




: tk.location.new_lc  ( uri line column -- tk.location )
    rot rot

    tk.location.new variable! location

    location tk.location.character!
    location tk.location.line!

    location @
;




: tk.location.inc  ( character tk.location --  )
    variable! location

    "\n" =
    if
        ( We're incrementing lines, so reset colum and increment the line. )
        0 location tk.location.character!
        location tk.location.line@ ++ location tk.location.line!
    else
        ( This isn't a new line, so we're just incrementing the character. )
        location tk.location.character@ ++ location tk.location.character!
    then
;




( Constants that represent the types of tokens we can find from the source text. )
0 constant tk.token_type:word
1 constant tk.token_type:comment
2 constant tk.token_type:string
3 constant tk.token_type:number
4 constant tk.token_type:eos


( When source code read in, the text is broken up into a list of tokens that represent that text. )
# tk.token
    location    ( Where and in what file was this token found?  This should be a tk.location. )
    line_count  ( How many lines of text does this token span, usually 1. )
    type        ( Is this a word, comment, string, or number? )
    contents    ( Can be text, a number, or a list of tokens in the case of comments. )
;




: tk.token.location!    tk.token.location    swap @ #! ;
: tk.token.line_count!  tk.token.line_count  swap @ #! ;
: tk.token.type!        tk.token.type        swap @ #! ;
: tk.token.contents!    tk.token.contents    swap @ #! ;

: tk.token.location@    tk.token.location    swap @ .s #@ ;
: tk.token.line_count@  tk.token.line_count  swap @ #@ ;
: tk.token.type@        tk.token.type        swap @ #@ ;
: tk.token.contents@    tk.token.contents    swap @ #@ ;




: tk.token.is_word?
    tk.token.type@ tk.token_type:word =
;


: tk.token.is_comment?
    tk.token.type@ tk.token_type:comment =
;


: tk.token.is_string?
    tk.token.type@ tk.token_type:string =
;


: tk.token.is_number?
    tk.token.type@ tk.token_type:number =
;


: tk.token.is_eos?
    tk.token.type@ tk.token_type:eos =
;




: tk.token.location.line@  ( token_index -- token_line )
    @ variable! token

    tk.location.line tk.token.location token @ #@ #@
;



: tk.token.location.character@  ( token_index -- token_character )
    @ variable! token

    tk.location.character tk.token.location token @ #@ #@
;




( This buffer holds the source code during tokenization and acts as a source code stream. )
# tk.buffer
    location  ( Where logically are we in the source text, as in line/character. )
    index     ( Actual location we are in the source string. )
    source    ( The source code itself, represented as a string. )
;




: tk.buffer.location!  tk.buffer.location  swap @ #! ;
: tk.buffer.index!     tk.buffer.index     swap @ #! ;
: tk.buffer.source!    tk.buffer.source    swap @ #! ;

: tk.buffer.location@  tk.buffer.location  swap @ #@ ;
: tk.buffer.index@     tk.buffer.index     swap @ #@ ;
: tk.buffer.source@    tk.buffer.source    swap @ #@ ;




( Create a new source code buffer from the file's uri and contents. )
: tk.buffer.new  ( uri source_code -- tk.buffer )
    tk.buffer.new variable! source_buffer

                    source_buffer tk.buffer.source!
    tk.location.new source_buffer tk.buffer.location!
    0               source_buffer tk.buffer.index!

    source_buffer @
;



( Check to make sure the pointer isn't at the end of the stream. )
: tk.buffer.is_eos?  ( tk.buffer_var -- is_eos? )
    @ variable! source_buffer

    source_buffer tk.buffer.index@
    source_buffer tk.buffer.source@ string.size@

    >=
;




: tk.buffer.peek  ( tk.buffer -- next_char )
    @ variable! source_buffer

    source_buffer tk.buffer.is_eos?
    if
        " "
    else
        source_buffer tk.buffer.index@
        source_buffer tk.buffer.source@

        string.[]@
    then
;




: tk.buffer.next  ( tk.buffer -- next_char )
    @ variable! source_buffer

    source_buffer tk.buffer.is_eos?
    if
        " "
    else
        source_buffer tk.buffer.peek
        source_buffer dup tk.buffer.index@ ++ swap tk.buffer.index!

        dup source_buffer tk.buffer.location@ tk.location.inc
    then
;




: tk.is_whitespace_char  ( character -- is_whitespace )
    variable! next_char

    next_char @ "\n" =
    next_char @ "\t" =
    ||
    next_char @ " " =
    ||
;




: tk.buffer.skip_whitespace  ( tk.buffer -- )
    @ variable! source_buffer

    begin
        source_buffer tk.buffer.is_eos? false =
        source_buffer tk.buffer.peek tk.is_whitespace_char
        &&
    while
        source_buffer tk.buffer.next drop
    repeat
;




: tk.buffer.read_token_text
    @ variable! source_buffer
    "" variable! new_text

    begin
        source_buffer tk.buffer.is_eos? false =
        source_buffer tk.buffer.peek tk.is_whitespace_char false =
        &&
    while
        new_text @ source_buffer tk.buffer.next + new_text !
    repeat

    new_text @
;



: tk.buffer.read_comment_tokens  ( start_location tk.buffer -- n_lines comment_tokens )
    @ variable! source_buffer
    variable! start_location
    1 [].new variable! comment_list

    #.new tk.token {
        location -> start_location @ ,
        line_count -> 1 ,
        type -> tk.token_type:comment ,
        contents -> "("
    }
    comment_list [ 0 ]!!

    variable current_location
    variable current_text

    begin
        source_buffer tk.buffer.skip_whitespace
        source_buffer tk.buffer.location@ value.copy current_location !
        source_buffer tk.buffer.read_token_text current_text !

        current_text "" <>
        if
            comment_list [].size++!!

            #.new tk.token {
                location -> current_location @ ,
                line_count -> 1 ,
                type -> tk.token_type:comment ,
                contents -> current_text @
            }
            comment_list [ comment_list @ [].size@ -- ]!!
        then

        current_text @ ")" =
        source_buffer tk.buffer.is_eos? true =
        ||
    until

    tk.token.location comment_list [ comment_list [].size@@ -- ]@@ #@ tk.location.line swap #@
    variable! last_line

    1 variable! n_lines

    start_location tk.location.line@ last_line @ <>
    if
        last_line @ start_location tk.location.line@ - n_lines !
    then

    n_lines @
    comment_list @
;




: tk.buffer.read_string  ( starting_text tk.buffer_variable -- n_lines string )
    @ variable! source_buffer
    variable! string

    string @ string.size@ -- string @ string.[]@
    "\""
    =
    if
        1
        string @
    else
        variable next_char
        1 variable! n_lines

        begin
            source_buffer tk.buffer.is_eos? '
            next_char @ "\"" <>

            &&
        while
            source_buffer tk.buffer.next next_char !

            next_char @
            case
                "\\" of next_char @ source_buffer tk.buffer.next + next_char ! endof
                "\n" of "\\n" next_char ! n_lines ++! endof
            endcase

            string @ next_char @ + string !
        repeat

        n_lines @
        string @
    then
;




: tk.is_numeric? ( character -- bool )
    variable! next_char

    next_char @ "0" >=
    next_char @ "9" <=
    &&

    next_char @ "." =
    ||

    next_char @ "e" =
    ||
;




: tk.try_as_number ( text -- text_or_number token_type )
    variable! text
    tk.token_type:word variable! token_type

    true variable! is_a_number
    0 variable! index
    text @ string.size@ variable! count

    begin
        index @ count @ <
    while
        index @ text @ string.[]@ tk.is_numeric? true <>
        if
            false is_a_number !
            break
        then

        index ++!
    repeat

    is_a_number @
    if
        text @ string.to_number text !
        tk.token_type:number token_type !
    then

    text @
    token_type @
;




( Extract a new token from out input text stream buffer. )
: tk.buffer.read_token  ( tk_buffer_variable -- tk.token )
    @ variable! source_buffer

    source_buffer tk.buffer.skip_whitespace

    ( Get our starting location from the buffer. )
    source_buffer tk.buffer.location@ value.copy variable! start_location
    1 variable! n_lines
    variable token_type

    ( Read a word of text from the buffer. )
    source_buffer tk.buffer.read_token_text variable! text

    text @ "" =
    if
        tk.token_type:eos token_type !
    else
        text @ "(" =
        if
            start_location @ source_buffer tk.buffer.read_comment_tokens

            text !
            n_lines !

            tk.token_type:comment token_type !
        else
            0 text @ string.[]@ "\"" =
            if
                text @ source_buffer tk.buffer.read_string

                text !
                n_lines !
                tk.token_type:string token_type !
            else
                0 text @ string.[]@ tk.is_numeric?
                if
                    text @ tk.try_as_number token_type ! text !
                else
                    tk.token_type:word token_type !
                then
            then
        then
    then

    ( Build a token with all the pieces of information we have. )
    #.new tk.token {
        location -> start_location @ ,
        line_count -> n_lines @ ,
        type -> token_type @ ,
        contents -> text @
    }
;




( Like the source buffer, tk.buffer, but in this case represents source text that's been fully )
( tokenized. This token list supports being treated like a stream, just like the tk.buffer, )
( allowing for some standard parsing algorithms to be applied to the token list. )
# tk.token_list
    items     ( The actual tokens themselves. )
    position  ( Where in the token list are we currently reading from? )
;


: tk.token_list.items!     tk.token_list.items     swap @ #! ;
: tk.token_list.position!  tk.token_list.position  swap @ #! ;

: tk.token_list.items@     tk.token_list.items     swap @ #@ ;
: tk.token_list.position@  tk.token_list.position  swap @ #@ ;



( Check the indexed comment against the base comment tokens, are these two comments directly )
( adjacent to each other? )
: tk.token_list.next_comment_is_adjacent?  ( base_token next_index tokens -- is_adjacent? )
    @ variable! tokens
    variable! index
    variable! base_token

    variable next_token

    variable base_end
    variable next_start

    index @ tokens @ [].size@ <
    if
        tokens [ index @ ]@@ next_token !

        next_token tk.token.is_comment?
        if
            base_token tk.token.contents@ [ base_token tk.token.contents@ [].size@ -- ]@ base_end !
            next_token tk.token.contents@ [ 0 ]@ next_start !

            next_start tk.token.location.line@
            base_end tk.token.location.line@
            -

            1 <=
        else
            false
        then
    else
        false
    then
;



( Given a base index, iterate through the next tokens, and if those tokens are adjacent comments, )
( combine them into one larger comment. )
: tk.token_list.combine_consecutive_comments  ( base_index tokens -- num_combined )
    variable! tokens
    variable! base_index

    tokens @ [].size@ variable! size
    tokens [ base_index @ ]@@ variable! base_token

    base_index @ ++ variable! index

    variable next_token

    begin
        index @ size @ <
        dup .cr
        base_token @ index @ tokens tk.token_list.next_comment_is_adjacent?
        &&
    while
        tokens [ index @ ]@@ next_token !

        base_token tk.token.location.line@ next_token tk.token.location.line@
        <>
        if
            base_token tk.token.line_count@ next_token tk.token.line_count@ +
            base_token tk.token.line_count!
        then

        base_token tk.token.contents@ next_token tk.token.contents@ [].+
        base_token tk.token.contents!

        index ++!
    repeat

    index @ base_index @ - dup .cr
;



( Go through the token list and compress all adjacent comment tokens into a single comment token. )
: tk.token_list.compress_comments
    variable! tokens
    tokens @ [].size@ [].new variable! new_list

    variable index
    variable added

    variable token
    variable next_token

    begin
        index @
        tokens @ [].size@
        <
    while
        tokens [ index @ ]@@ token !

        index @ ++ tokens @ [].size@ <
        if
            tokens [ index @ ++ ]@@ next_token !

            token tk.token.is_comment?
            next_token tk.token.is_comment?
            &&
            if
                index @ tokens @ tk.token_list.combine_consecutive_comments
                -- index @ + index !
            then
        then

        token @ new_list [ added @ ]!!
        added ++!

        index ++!
    repeat

    added @ new_list @ [].size!
    new_list @
;




: tk.token_list.tokenize  ( uri source_code -- tk.token_list )
    tk.buffer.new variable! source_buffer
    0 [].new variable! token_list
    variable token

    begin
        source_buffer tk.buffer.is_eos? '
    while
        source_buffer tk.buffer.read_token token !

        token tk.token.type@ tk.token_type:eos <>
        if
            token_list [].size++!!
            token @ token_list [ token_list [].size@@ -- ]!!
        then
    repeat

    token_list @ tk.token_list.compress_comments token_list !

    #.new tk.token_list {
        items -> token_list @ ,
        position -> 0
    }
;




: tk.token_list.is_eos?  ( token_list_variable -- is_eos? )
    @ variable! token_list

    token_list tk.token_list.position@
    token_list tk.token_list.items@ [].size@

    >=
;




( Peek into the token list and read out the current token without advancing the current position. )
: tk.token_list.peek  ( token_list_variable -- current_token )
    @ variable! token_list

    token_list tk.token_list.is_eos?
    if
        #.new tk.token {
            type -> tk.token_type:eos
        }
    else
        token_list tk.token_list.items@ [ token_list tk.token_list.position@ ]@
    then
;




( Get the next token, and advance the current position. )
: tk.token_list.next  ( token_list_variable -- current_token )
    @ variable! token_list

    token_list tk.token_list.peek variable! next_token

    token_list tk.token_list.position@ ++
    token_list tk.token_list.position!

    next_token @
;




( Reset back to the beginning of the stream. )
: tk.token_list.reset
    @ variable! token_list

    0 token_list tk.token_list.position!
;
