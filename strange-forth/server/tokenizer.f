
( Implementation of the source text tokenizer and supporting structures. Everything in this file )
( falls under the tk. namespace. )


( Represent a given location within the source text. )
# tk.location
    uri        ( The uri we get from vscode for the file being edited. )
    line       ( Zero based line that we are on. )
    character  ( Zero based column that we are on. )
;


( Overbidden constructor that takes a uri and initializes the line and character to default. )
: tk.location.new  ( uri -- tk.location )
    tk.location.new variable! location

    location tk.location.uri!!

    location @
;




( Increment the location based on the character encountered. )
: tk.location.inc  ( character tk.location --  )
    variable! location

    "\n" =
    if
        ( We're incrementing lines, so reset colum and increment the line. )
        0 location tk.location.character!!
        location tk.location.line@@ ++ location tk.location.line!!
    else
        ( This isn't a new line, so we're just incrementing the character. )
        location tk.location.character@@ ++ location tk.location.character!!
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




( Is the token variable holding a word token? )
: tk.token.is_word?  ( token_variable -- bool )
    tk.token.type@@ tk.token_type:word =
;




( Is the token variable holding a word comment? )
: tk.token.is_comment?  ( token_variable -- bool )
    tk.token.type@@ tk.token_type:comment =
;




( Is the token variable holding a string token? )
: tk.token.is_string?  ( token_variable -- bool )
    tk.token.type@@ tk.token_type:string =
;




( Is the token variable holding a number token? )
: tk.token.is_number?  ( token_variable -- bool )
    tk.token.type@@ tk.token_type:number =
;




( Is the token variable holding a end of stream token? )
: tk.token.is_eos?  ( token_variable -- bool )
    tk.token.type@@ tk.token_type:eos =
;




: tk.token.location.line@@  ( token_index -- token_line )
    @ variable! token

    tk.location.line tk.token.location token @ #@ #@
;



: tk.token.location.character@@  ( token_index -- token_character )
    @ variable! token

    tk.location.character tk.token.location token @ #@ #@
;




( This buffer holds the source code during tokenization and acts as a source code stream. )
# tk.buffer
    location  ( Where logically are we in the source text, as in line/character. )
    index     ( Actual location we are in the source string. )
    source    ( The source code itself, represented as a string. )
;




( Create a new source code buffer from the file's uri and contents. )
: tk.buffer.new  ( uri source_code -- tk.buffer )
    tk.buffer.new variable! source_buffer

                    source_buffer tk.buffer.source!!
    tk.location.new source_buffer tk.buffer.location!!
    0               source_buffer tk.buffer.index!!

    source_buffer @
;



( Check to make sure the pointer isn't at the end of the stream. )
: tk.buffer.is_eos?  ( tk.buffer_var -- is_eos? )
    @ variable! source_buffer

    source_buffer tk.buffer.index@@
    source_buffer tk.buffer.source@@ string.size@

    >=
;




( Peek into the string buffer without advancing the pointer. )
: tk.buffer.peek  ( tk.buffer -- next_char )
    @ variable! source_buffer

    source_buffer tk.buffer.is_eos?
    if
        " "
    else
        source_buffer tk.buffer.index@@
        source_buffer tk.buffer.source@@

        string.[]@
    then
;




( Get the next character from the text stream and advance the current location pointer. )
: tk.buffer.next  ( tk.buffer -- next_char )
    @ variable! source_buffer

    source_buffer tk.buffer.is_eos?
    if
        " "
    else
        source_buffer tk.buffer.peek
        source_buffer dup tk.buffer.index@@ ++ swap tk.buffer.index!!

        dup source_buffer tk.buffer.location@@ tk.location.inc
    then
;




( Is the given character considered whitespace? )
: tk.is_whitespace_char  ( character -- is_whitespace )
    variable! next_char

    next_char @ "\n" =
    next_char @ "\t" =
    ||
    next_char @ " " =
    ||
;




( Skip past whitespace in the source buffer. )
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




( Read a textual token from the source stream. )
: tk.buffer.read_token_text  ( source_buffer_variable -- new_token_text )
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




( Comment tokens are a bit different than other tokens.  The contents of a top level token is )
( list of sub-tokens that actually comprise the comment, including bracket tokens. )
( The contents of the sub-tokens actually contain text. )
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
        source_buffer tk.buffer.location@@ value.copy current_location !
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

    start_location tk.location.line@@ last_line @ <>
    if
        last_line @ start_location tk.location.line@@ - n_lines !
    then

    n_lines @
    comment_list @
;




( Read a string literal from the source buffer.  We are given a token's worth of starting text and )
( we'll append to that text until we find the closing quote.  Unlike in the actual language this )
( version will happily consume the string across new lines. )
: tk.buffer.read_string  ( starting_text tk.buffer_variable -- num_lines string )
    @ variable! source_buffer
    variable! string

    ( Check to see if the starting text has the closing quote.  Note that if the quote is the only )
    ( character in the text, then it doesn't count as a closing quote. )
    string @ string.size@ 1  >
    string @ string.size@ -- string @ string.[]@  "\""  =
    &&
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




( Is the character a numeric digit? )
: tk.is_digit?
    variable! next_char

    next_char @ "0" >=
    next_char @ "9" <=
    &&
;




( This this character look like it's probably part of a numeric string? )
: tk.is_numeric?  ( character -- bool )
    variable! next_char

    next_char @ tk.is_digit?

    next_char @ "." =
    ||

    next_char @ "e" =
    ||

    next_char @ "-" =
    ||
;




( Try to handle the given text as a numeric literal.  If it does look like a numeric literal )
( convert to a number now. )
: tk.try_as_number ( text -- text_or_number token_type )
    variable! text
    tk.token_type:word variable! token_type

    true variable! is_a_number?
    false variable! found_digit?

    0 variable! index
    text @ string.size@ variable! count

    variable next_char

    begin
        index @ count @ <
    while
        index @  text @  string.[]@  next_char !

        next_char @ tk.is_numeric? true <>
        if
            false is_a_number? !
            break
        then

        next_char @  tk.is_digit?
        if
            true found_digit?
        then

        index ++!
    repeat

    is_a_number? @
    found_digit? @
    &&
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
    source_buffer tk.buffer.location@@ value.copy variable! start_location
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
            base_token tk.token.contents@@ [ base_token tk.token.contents@@ [].size@ -- ]@ base_end !
            next_token tk.token.contents@@ [ 0 ]@ next_start !

            next_start tk.token.location.line@@
            base_end tk.token.location.line@@
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
        base_token @ index @ tokens tk.token_list.next_comment_is_adjacent?
        &&
    while
        tokens [ index @ ]@@ next_token !

        base_token tk.token.location.line@@ next_token tk.token.location.line@@
        <>
        if
            base_token tk.token.line_count@@ next_token tk.token.line_count@@ +
            base_token tk.token.line_count!!
        then

        base_token tk.token.contents@@ next_token tk.token.contents@@ [].+
        base_token tk.token.contents!!

        index ++!
    repeat

    index @  base_index @  -
;



( Go through the token list and compress all adjacent comment tokens into a single comment token. )
: tk.token_list.compress_comments  ( token_list -- compressed_list )
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

    ( Resize the list back down to how many tokens are actually in the list. )
    added @ new_list @ [].size!
    new_list @
;




( Take a source text from the language client and break it down into a token list for later )
( processing. )
: tk.tokenize  ( uri source_code -- token_list )
    tk.buffer.new variable! source_buffer
    0 [].new variable! token_list
    variable token

    begin
        source_buffer tk.buffer.is_eos? '
    while
        source_buffer tk.buffer.read_token token !

        token tk.token.type@@ tk.token_type:eos <>
        if
            token_list [].size++!!
            token @ token_list [ token_list [].size@@ -- ]!!
        then
    repeat

    token_list @ tk.token_list.compress_comments
;
