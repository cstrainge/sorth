
{}.new variable! ds.document_store  ( All documents are stored here, indexed by uri. )

{}.new variable! ds.symbol_table   ( A copy of all of the symbols we know about from all loaded )
                                   ( documents in our collection. )



( Definition of an editable document and all of it's supporting information.  It is this document )
( object that is stored in the main ds.document_store. )
# ds.document
    uri          ( VS Code uri for this file. )
    version      ( Version of this document. )
    token_list   ( Tokenized version of the source text. )
    symbols      ( Table of symbols extracted from the document. )
    contents     ( The actual source text of the document. )
;


: ds.document.uri!             ds.document.uri            swap @ #! ;
: ds.document.version!         ds.document.version        swap @ #! ;
: ds.document.token_list!      ds.document.token_list     swap @ #! ;
: ds.document.symbols!         ds.document.symbols        swap @ #! ;
: ds.document.contents!        ds.document.contents       swap @ #! ;

: ds.document.uri@             ds.document.uri            swap @ #@ ;
: ds.document.version@         ds.document.version        swap @ #@ ;
: ds.document.token_list!      ds.document.token_list     swap @ #! ;
: ds.document.symbols!         ds.document.symbols        swap @ #! ;
: ds.document.contents@        ds.document.contents       swap @ #@ ;


: ds.document.new  ( uri version contents -- ds.document )
    ds.document.new variable! document

    document ds.document.contents!
    document ds.document.version!
    document ds.document.uri!

    {}.new ds.document.symbols!

    document @
;




: ds.document.generate_tokens  ( document_variable -- )
    @ variable! document

    document ds.document.uri@  document ds.document.contents@  tk.token_list.tokenize
    document ds.document.token_list!
;




: ds.document.scan_keyword  ( start_index name token_list -- [found_index] bool )
    variable! tokens
    variable! name
    variable! index

    false variable! was_found?

    tokens [].size@@ variable! size
    variable token

    begin
        index @  size @  <
    while
        tokens [ token @ ]@@  token !

        token tk.token.contents@  name @  =
        if
            true was_found? !
            break
        then

        token tk.token.contents  ";"  =
        if
            break
        then

        token ++!
    repeat

    was_found? @ dup
    if
        index @
        swap
    then
;




: ds.document.scan_keyword_string  ( start_index name token_list -- [string] was_found? )
    variable! tokens
    variable! name
    variable! index

    tokens [].size@@ variable! size

    index @  name @  tokens @  ds.document.scan_keyword  variable! was_found?
    variable found_index
    variable token

    was_found? @
    if
        ++ found_index !

        found_index @  size @  <
        if
            tokens [ found_index @ ]@@  token !

            token tk.token.is_string?
            if
                token tk.token.contents@
                true was_found !
            else
                false was_found !
            then
        then
    then

    was_found? @
;




: ds.document.scan_for_previous_comment
    variable! tokens
    variable! index
    tokens [ index @ ]@@ variable! token

    index @ --  variable! previous_index
    variable previous_token

    previous_index 0>=
    if
        tokens [ previous_index @ ]@@  previous_token !

        previous_token tk.token.is_comment?
        if
            previous_token tk.token.contents@ [ previous_token tk.token.contents@ [].size@ -- ]@

            tk.token.location swap #@
            tk.location.line swap #@

            token tk.token.location.line@ --

            =
        else
            false
        then
    else
        false
    then
;




: ds.document.scan_for_next_comment
    variable! tokens
    variable! index

    tokens [ index @ ]@@ variable! token

    index @ ++  variable! next_index
    variable previous_token

    next_index  tokens [].size@@  <
    if
        tokens [ next_index @ ]@@  next_token !

        next_token tk.token.is_comment?
        if
            next_token tk.token.location.line@
            token tk.token.location.line@
            =
        else
            false
        then
    else
        false
    then
;




: ds.document.tokenize_signature  ( text -- tk.tokens )
    "string"  swap  tk.token_list.tokenize  tk.token_list.items swap #@
;




: ds.document.stringify_comment
    variable! tokens
    tokens [].size@@ variable! size

    variable index
    variable token

    "" variable! text

    begin
        index @  size @  <
    while
        tokens [ index @ ]@@  token !

        token tk.token.contents@  "("  <>
        token tk.token.contents@  ")"  <>
        &&
        if
            text @  token tk.token.contents@  +

            index @  size --  <
            if
                text @  " "  +
            then
        then

        index ++!
    repeat

    text @
;




: ds.document.signature_item
    variable! list
    variable! index

    index @  list [].size@@  <
    if
        "| "  tk.token.contents list [ index@ ]@@ #@  +  " |" +
    else
        "|    |"
    then
;




: ds.document.signature_markdown_table
    variable! pre_list
    variable! post_list

    variable table_text

    pre_list [].size@@  variable! size
    variable index

    post_list [].size@@ dup  size @  >
    if
        size !
    else
        drop
    then

    "| Pre  |       | Post |\n| :--- | :---: | :--- |\n" table_text !

    size @ --  index !

    begin
        index 0>=
    while
        table_text @
        index @  pre_list @   ds.document.signature_item  +

        index @ 0=
        if
            " --- " +
        else
            "     " +
        then

        index @  post_list @  ds.document.signature_item  +  "\n" +

        index --!
    repeat

    table_text @
;




: ds.document.signature_to_markdown
    variable! tokens

    0 [].new variable! pre_list
    0 [].new variable! post_list

    false variable! found_marker?

    variable index
    variable token

    begin
        index @  size @  <
    while
        tokens [ index @ ]@@  token !

        token tk.token.contents@
        case
            "(" of endcase
            ")" of endcase

            "--" of true found_marker? ! endcase

            found_marker?
            if
                pre_list [].size++!!
                token pre_list [ pre_list [].size!! -- ]!!
            else
                post_list [].size++!!
                token post_list [ post_list [].size!! -- ]!!
            then
        endcase

        index ++!
    repeat

    found_marker?
    if
        pre_list @  post_list @  ds.document.signature_markdown_table
    else
        pre_list @ ds.document.stringify_comment
    then
;




( We found a word definition in the document.  Scan the surrounding area for any information we )
( find for this word and if found, record in the symbol table for this document. )
: ds.document.symbolize_new_word  ( token_index document_variable -- )
    @ variable! document
    variable! index

    tk.token_list.items  document ds.document.token_list@  #@  variable! tokens
    variable name

    index @ ++  tokens [].size@@  <
    if
        tokens [ index @ ++ ]@@ name !

        index @ "description:" tokens @ ds.document.scan_keyword_string variable! description
        index @ "signature:" tokens @ ds.document.scan_keyword_string variable! signature
        index @ "immediate" tokens @ ds.document.scan_keyword variable! is_immediate

        description @ "" =
        if
            index @ tokens @ ds.document.scan_for_previous_comment
            if
                tk.token.contents swap #@  ds.document.stringify_comment  description !
            then
        then

        signature @ "" =
        if
            index @ ++ tokens @ ds.document.scan_for_next_comment
            if
                tk.token.contents swap #@  ds.document.signature_to_markdown  signature !
            then
        else
            signature @ ds.document.tokenize_signature
            ds.document.signature_to_markdown
            signature !
        then

        #.new word_info {
            is_immediate -> is_immediate @ ,
            is_scripted -> true ,
            description -> description @ ,
            signature -> signature @ ,
            handler_index -> -1
        }
        document ds.document.symbols@ { name @ }!
    then
;




: ds.document.symbolize_new_struct  ( token_index document_variable -- )
;




: ds.document.symbolize_new_variable  ( token_index document_variable -- )
;




: ds.document.symbolize_new_constant  ( token_index document_variable -- )
;




: ds.document.generate_symbols  ( document_variable -- )
    @ variable! document
    document ds.document.token_list@ variable! tokens

    variable next_token
    variable index

    begin
        tokens tk.token_list.items@ [ index @ ]@  next_token !

        next_token tk.token.is_eos? '
    while
        next_token tk.token.is_word?
        if
            next_token tk.token.contents@
            case
                ":"         of  index @ document ds.document.symbolize_new_word      endof
                "#"         of  index @ document ds.document.symbolize_new_struct    endof
                "variable"  of  index @ document ds.document.symbolize_new_variable  endof
                "variable!" of  index @ document ds.document.symbolize_new_variable  endof
                "constant"  of  index @ document ds.document.symbolize_new_constant  endof
            endcase
        then

        index ++!
    until
;




: ds.document.character_in_word_token?  ( character_index tk.token -- is_inside? )
    variable! token
    variable! character_index

    variable start_character
    variable end_character

    token tk.token.is_word?
    if
        token tk.token.location.character@  start_character !
        start_character @  token.contents@ string.size@ --  end_character !

        character_index @  start_character @  >=
        character_index @  end_character @    <=
        &&
    else
        false
    then
;




: ds.document.scan_for_word  ( tk.location document_variable -- [tk.token] was_found? )
    @ variable! document
    variable! location

    document ds.document.token_list@ variable! tokens
    tokens tk.token_list.items@  tokens !

    variable index
    variable token
    false variable! was_found?

    begin
        index @  tokens [].size@  <
    while
        tokens [ index @ ]@@  token !

        location tk.location.line@  token tk.token.location.line@  =
        if
            location tk.location.character@  token @  ds.document.character_in_word_token?
            if
                true was_found? !
                break
            then
        else
            location tk.location.line@  token tk.token.location.line@  >
            if
                break
            then
        then

        index ++!
    repeat

    was_found? @
    if
        token @
        true
    else
        false
    then
;




: ds.regenerate_master_symbol_list
    {}.new ds.symbol_table !  ( Clear out the table completely. )

    ds.symbol_table @  ls.standard_words  {}.+ drop

    : ds.regeneration_document_iterator
        variable! document
        variable! uri

        ds.symbol_table @  document ds.document.symbols@  {}.+ drop
    ;

    ` ds.regeneration_document_iterator ds.document_store @ {}.iterate
;




: ds.insert_document  ( uri version contents -- )
    ds.document.new variable! new_document

    new_document @ ds.document_store { new_document ds.document.uri@ }!!
    new_document ds.document.generate_tokens
    new_document ds.document.generate_symbols

    ds.regenerate_master_symbol_list
;




: ds.find_document  ( uri -- [document] was_found )
    variable! uri

    uri @ ds.document_store @ {}?
    if
        ds.document_store { uri @ }@@
        true
    else
        false
    then
;




: ds.find_word_info  ( word_name -- [found_word] was_found? )
    variable! name

    name @ ds.symbol_table @ {}?
    if
        ds.symbol_table { name @ }@@
        true
    else
        false
    then
;
