
( Implementation of the language server's document store.  This is where we keep all loaded )
( documents as well as all useful information extracted from said documents. )




( Internal document store state. )
{}.new variable! ds.document_store  ( All documents are stored here, indexed by uri. )

variable ds.symbol_table            ( A copy of all of the symbols we know about from all loaded )
                                    ( documents in our collection. )

variable ds.base_symbols            ( A copy of the symbols found in the standard library. )




( Constants for the various symbol types we can find in a source document. )
0 constant ds.document.symbol_type:word
1 constant ds.document.symbol_type:struct
2 constant ds.document.symbol_type:variable
3 constant ds.document.symbol_type:constant


( Represent one of the words found defined in the source code. )
# ds.document.symbol
    is_immediate  ( Is this word immediate? )
    description   ( The word's description. )
    signature     ( The word's data stack signature. )
    location      ( Where this word was found. )
    type          ( One of ds.document.symbol_type:* constants. )
;


: ds.document.symbol.is_immediate!  ds.document.symbol.is_immediate  swap @ #! ;
: ds.document.symbol.description!   ds.document.symbol.description   swap @ #! ;
: ds.document.symbol.signature!     ds.document.symbol.signature     swap @ #! ;
: ds.document.symbol.location!      ds.document.symbol.location      swap @ #! ;
: ds.document.symbol.type!          ds.document.symbol.type          swap @ #! ;

: ds.document.symbol.is_immediate@  ds.document.symbol.is_immediate  swap @ #@ ;
: ds.document.symbol.description@   ds.document.symbol.description   swap @ #@ ;
: ds.document.symbol.signature@     ds.document.symbol.signature     swap @ #@ ;
: ds.document.symbol.location@      ds.document.symbol.location      swap @ #@ ;
: ds.document.symbol.type@          ds.document.symbol.type          swap @ #@ ;




( Definition of an editable document and all of it's supporting information.  It is this document )
( object that is stored in the main ds.document_store. )
# ds.document
    uri          ( VS Code uri for this file. )
    version      ( Version of this document. )
    token_list   ( Tokenized version of the source text. )
    symbols      ( Table of symbols extracted from the document. )
    contents     ( The actual source text of the document. )
;


: ds.document.uri!         ds.document.uri         swap @ #! ;
: ds.document.version!     ds.document.version     swap @ #! ;
: ds.document.token_list!  ds.document.token_list  swap @ #! ;
: ds.document.symbols!     ds.document.symbols     swap @ #! ;
: ds.document.contents!    ds.document.contents    swap @ #! ;

: ds.document.uri@         ds.document.uri         swap @ #@ ;
: ds.document.version@     ds.document.version     swap @ #@ ;
: ds.document.token_list@  ds.document.token_list  swap @ #@ ;
: ds.document.symbols@     ds.document.symbols     swap @ #@ ;
: ds.document.contents@    ds.document.contents    swap @ #@ ;




( Construct a new instance of the structure ds.document.  This version takes the document's uri, )
( version, and source text. )
: ds.document.new  ( uri version contents -- ds.document )
    ds.document.new variable! document

    document ds.document.contents!
    document ds.document.version!
    document ds.document.uri!

    {}.new document ds.document.symbols!

    document @
;




( Take a comment token's sub-tokens and turn them into a string. )
: ds.document.stringify_comment  ( comment_contents -- string )
    variable! tokens
    tokens [].size@@ variable! size

    variable index
    variable token
    variable token_text

    "" variable! text

    begin
        index @  size @  <
    while
        tokens [ index @ ]@@  token !
        token tk.token.contents@  token_text !

        token_text @  "("  <>
        token_text @  ")"  <>
        &&
        if
            text @  token_text @  +  text !

            index @  size @ --  <
            if
                text @  " "  +  text !
            then
        then

        index ++!
    repeat

    text @
;



( Given a token index and a list of tokens, scan before that index and look for a comment token )
( that's logically adjacent to that token.  That is, is on the same or previous line of text as )
( the original token. )
: ds.document.scan_for_previous_comment  ( base_index tokens -- [token] was_found? )
    variable! tokens
    variable! base_index

    tokens [ base_index @ ]@@ variable! base_token

    index @ --  variable! previous_index
    variable previous_token
    variable end_token

    previous_index @  0>=
    if
        tokens [ previous_index @ ]@@  previous_token !

        previous_token tk.token.is_comment?
        if
            previous_token tk.token.contents@  [ previous_token tk.token.contents@ [].size@ -- ]@
            end_token !

            end_token tk.token.location.line@  base_token tk.token.location.line@     =
            end_token tk.token.location.line@  base_token tk.token.location.line@ --  =
            ||
            if
                previous_token @
                true
            else
                false
            then
        else
            false
        then
    else
        false
    then
;




( Given a token index and a list of tokens, scan after that index and look for a comment token )
( that's logically adjacent to that token.  That is, is on the same line of text as the original )
( token. )
: ds.document.scan_for_next_comment  ( base_index tokens -- [token] was_found? )
    variable! tokens
    variable! base_index

    tokens [].size@@  variable! size
    base_index @ ++  variable! next_index

    tokens [ index @ ]@@  variable! base_token
    variable next_token

    next_index @  size @  <
    if
        tokens [ next_index @ ]@@  next_token !

        next_token tk.token.is_comment?
        if
            next_token tk.token.location.line@  base_token tk.token.location.line@     =
            next_token tk.token.location.line@  base_token tk.token.location.line@ ++  =
            ||
            if
                next_token @
                true
            else
                false
            then
        else
            false
        then
    else
        false
    then
;




( Scan the tokens for a specific word token, keyword.  Starting at base_index and ending at )
( end_word.  Give "" to scan until end of the document. )
: ds.document.scan_for_keyword  ( base_index keyword end_word tokens -- [found_index] was_found? )
    variable! tokens
    variable! end_word
    variable! keyword
    variable! base_index

    tokens [].size@@  variable! size
    base_index @  variable! index
    variable next_token

    false variable! found_token?

    begin
        index @  size @  <
    while
        tokens [ index @ ]@@  next_token !

        next_token tk.token.is_word?
        if
            next_token tk.token.contents@  end_word @  <>
            if
                next_token tk.token.contents@  keyword @  =
                if
                    true found_token? !
                    break
                then
            else
                break
            then
        then

        index ++!
    repeat

    found_token? @  dup
    if
        index @
        swap
    then
;




( Scan the tokens for a specific word token, keyword.  Starting at base_index and ending at )
( end_word.  Give an empty string to scan until end of the document.  Then return the next token )
( if it is a string token.  If the next token is not a string token, the search is considered )
( failed. )
: ds.document.scan_for_keyword_string  ( base_index keyword end_word tokens -- [string] was_found? )
    variable! tokens
    variable! end_word
    variable! keyword
    variable! base_index

    tokens [].size@@  variable! size

    variable next_token

    base_index @ keyword @ end_word @ tokens @ ds.document.scan_for_keyword
    if
        ( Get the index of the token right after the found keyword index. )
        ++ base_index !

        base_index @  size @  <
        if
            tokens [ base_index @ ]@@  next_token !

            next_token tk.token.is_string?
            if
                next_token @
                true
            else
                false
            then
        else
            false
        then
    else
        false
    then
;




( Given a string surrounded in quotes, remove those quotes and return the new string. )
: ds.document.remove_string_quotes  ( string -- de-quoted_string )
    variable! string

    ( Note that we assume that the string is quoted and don't check. )

    1  0                        string @ string.remove  string !
    1  string string.size@@ --  string @ string.remove  string !

    string @
;




( Given a s string, break it up into tokens. )
: ds.document.tokenize_string  ( string -- token_list )
    "string"  swap  tk.tokenize
;




( Filter strings so that they don't interfere with Markdown formatting. )
: ds.document.filter_markdown  ( original -- formatted )
    variable! original
    "" variable! new

    original string.size@@ variable! size
    variable index

    variable next_char

    begin
        index @  size @  <
    while
        index @  original @  string.[]@  next_char !

        next_char @
        case
            "[" of  "\\\\["  next_char !  endof
            "]" of  "\\\\]"  next_char !  endof
            "<" of  "\\\\<"  next_char !  endof
            ">" of  "\\\\>"  next_char !  endof
        endcase

        new @  next_char @  +  new !

        index ++!
    repeat

    new @
;




( Take a list time and convert it to a markdown table item.  If the index isn't in the list an )
( empty table item will be generated instead. )
: ds.document.signature_table_item  ( index list -- table_item_string )
    variable! list
    variable! index

    index @  list [].size@@  <
    if
        "| "  tk.token.contents list [ index @ ]@@ #@  ds.document.filter_markdown  +  " |" +
    else
        "|    |"
    then
;




( Given a two lists of tokens, one for the stack previous to the word being called, and one for )
( the list of values to be left on the stack after being called. )
: ds.document.signature_markdown_table  ( pre_list post_list -- markdown_table )
    variable! post_list
    variable! pre_list

    variable table_text

    pre_list [].size@@  variable! max_size

    post_list [].size@@ dup  max_size @  >
    if
        max_size !
    else
        drop
    then

    max_size @  0=
    if
        1 max_size !
    then

    "| Pre  |       | Post |\\n| :--- | :---: | :--- |\\n" table_text !

    max_size @ --  variable! index

    begin
        index @  0>=
    while
        table_text @
        index @  pre_list @  ds.document.signature_table_item  +

        index @ 0=
        if
            " -- " +
        else
            "    " +
        then

        index @  post_list @  ds.document.signature_table_item  +  "\\n" +  table_text !

        index --!
    repeat

    table_text @
;




( Given a list of tokens that represents a word's signature go through those tokens and reformat )
( them as a markdown formatted string. )
: ds.document.signature_to_markdown  ( token_list -- markdown_string )
    variable! tokens
    tokens [].size@@  variable! size

    0 [].new variable! pre_list
    0 [].new variable! post_list

    ( 0 variable! size )

    false variable! found_marker?

    variable index
    variable token

    begin
        index @  size @  <
    while
        tokens [ index @ ]@@  token !

        token tk.token.contents@
        case
            "(" of endof
            ")" of endof

            "--" of true found_marker? ! endof
            found_marker? @ '
            if
                pre_list [].size++!!
                token @  pre_list [ pre_list [].size@@ -- ]!!
            else
                post_list [].size++!!
                token @  post_list [ post_list [].size@@ -- ]!!
            then
        endcase

        index ++!
    repeat

    found_marker? @
    if
        pre_list @  post_list @  ds.document.signature_markdown_table
    else
        pre_list @  ds.document.stringify_comment  ds.document.filter_markdown
    then
;




( Scan around a word's definition for it's description.  This can either be in a comment before )
( the word is defined, or in the word using the description: keyword. )
: ds.document.scan_for_description  ( start_index tokens -- description )
    variable! tokens
    variable! start_index
    "" variable! description

    start_index @ "description:" ";" tokens @ ds.document.scan_for_keyword_string
    if
        description !
        description tk.token.contents@  ds.document.remove_string_quotes
                                        ds.document.filter_markdown       description !
    else
        start_index @ tokens @ ds.document.scan_for_previous_comment
        if
            tk.token.contents swap #@  ds.document.stringify_comment
                                       ds.document.filter_markdown    description !
        then
    then

    description @
;




( Scan for a word's signature.  Either as a comment or by using the signature: keyword. )
: ds.document.scan_for_signature  ( start_index tokens -- signature )
    variable! tokens
    variable! start_index
    "" variable! signature

    start_index @ "signature:" ";" tokens @ ds.document.scan_for_keyword_string
    if
        signature !

        signature tk.token.contents@  ds.document.remove_string_quotes
                                      ds.document.tokenize_string
                                      ds.document.signature_to_markdown  signature !
    else
        start_index ++!
        start_index @ tokens @ ds.document.scan_for_next_comment
        if
            tk.token.contents swap #@  ds.document.signature_to_markdown  signature !
        then
    then

    signature @
;




( Scan the body of a word for a flag marking that word as immediate. )
: ds.document.scan_for_immediate_flag  ( start_index tokens -- was_found? )
    variable! tokens
    variable! start_index

    start_index @ "immediate" ";" tokens @ ds.document.scan_for_keyword_string

    if
        drop
        true
    else
        false
    then
;




( We found a word definition in the document.  Scan the surrounding area for any information we )
( find for this word and if found, record in the symbol table for this document. )
: ds.document.symbolize_new_word  ( token_index document_variable -- )
    @ variable! document
    variable! tokens
    variable! index

    variable name
    variable description
    variable signature
    false variable! is_immediate

    index @ ++  tokens [].size@@  <
    if
        tokens [ index @ ++ ]@@ name !

        name tk.token.is_word?
        if
            index @ tokens @ ds.document.scan_for_description  description !
            index @ tokens @ ds.document.scan_for_signature    signature !
            index @ tokens @ ds.document.scan_for_immediate_flag  is_immediate !

            #.new ds.document.symbol {
                is_immediate -> is_immediate @ ,
                description -> description @ ,
                signature -> signature @ ,
                location -> name tk.token.location@ ,
                type -> ds.document.symbol_type:word
            }
            document ds.document.symbols@ { name tk.token.contents@ }!
        then
    then
;




( Generate a symbol for a structure and store it in the document's symbol table. )
: ds.document.symbolize_new_struct
    @ variable! document
    variable! tokens
    variable! index
;




( Generate a symbol for a variable and store it in the document's symbol table. )
: ds.document.symbolize_new_variable
    @ variable! document
    variable! tokens
    variable! index
;




( Generate a symbol for a constant and store it in the document's symbol table. )
: ds.document.symbolize_new_constant
    @ variable! document
    variable! tokens
    variable! index
;




( Tokenize the given document's source code and store that token list in the document structure. )
: ds.document.generate_tokens  ( document_variable -- )
    @ variable! document

    document ds.document.uri@  document ds.document.contents@  tk.tokenize
    document ds.document.token_list!
;




( Given a document, and assuming it's token list is up to date scan through and generate a )
( collection of symbols that we found within that document.  All symbols are stored within the )
( document object itself. )
: ds.document.generate_symbols  ( document_variable -- )
    @ variable! document

    document ds.document.token_list@ variable! tokens
    tokens [].size@@ variable! size

    variable index
    variable next_token

    {}.new document ds.document.symbols!

    begin
        index @  size @  <
    while
        tokens [ index @ ]@@  next_token !

        next_token tk.token.is_word?
        if
            next_token tk.token.contents@
            case
                ":"         of  index @ tokens @ document ds.document.symbolize_new_word      endof
                "#"         of  index @ tokens @ document ds.document.symbolize_new_struct    endof
                "variable"  of  index @ tokens @ document ds.document.symbolize_new_variable  endof
                "variable!" of  index @ tokens @ document ds.document.symbolize_new_variable  endof
                "constant"  of  index @ tokens @ document ds.document.symbolize_new_constant  endof
            endcase
        then

        index ++!
    repeat
;




( Regenerate our symbol table from the system collection and all of the currently loaded )
( documents. )
: ds.regenerate_master_symbol_list  ( -- )
    : ds.regeneration_document_iterator
        variable! document
        variable! uri

        ds.symbol_table @              is_value_hash_table?
        document ds.document.symbols@  is_value_hash_table?
        &&
        if
            ds.symbol_table @  document ds.document.symbols@  {}.+ drop
        then
    ;

    {}.new ds.symbol_table !
    ds.symbol_table @  ds.base_symbols @  {}.+ drop

    ` ds.regeneration_document_iterator ds.document_store @ {}.iterate
;




( Convert the collection of word_info structs into our ds.document.symbol and create a base set of )
( symbols as defined by the standard library. )
: ds.add_std_symbols  ( system_word_table -- )
    variable! system_words

    : ds.add_std_symbols_iterator
        variable! info
        variable! name

        #.new ds.document.symbol {
            is_immediate -> word_info.is_immediate info @ #@ ,
            description -> word_info.description info @ #@ ,
            signature ->  word_info.signature info @ #@  ds.document.tokenize_string
                                                         ds.document.signature_to_markdown ,
            type -> ds.document.symbol_type:word
        }
        ds.base_symbols { name @ }!!
    ;

    {}.new  ds.base_symbols !
    ` ds.add_std_symbols_iterator system_words @ {}.iterate

    ds.base_symbols value.copy  ds.symbol_table !
;




( Create an store a new document within our document store.  All extra information needed from the )
( source text is also generated at this time.  For example a collection of symbols as defined )
( within the document. )
: ds.insert_document  ( uri version contents -- )
    ds.document.new variable! new_document

    new_document @ ds.document_store { new_document ds.document.uri@ }!!

    new_document ds.document.generate_tokens
    new_document ds.document.generate_symbols

    ds.regenerate_master_symbol_list
;




( Given a VS Code uri, search our document collection and try to return the document in question. )
: ds.find_document  ( uri -- [document] was_found? )
    variable! uri

    uri @  ds.document_store @  {}?
    if
        ds.document_store { uri @ }@@
        true
    else
        false
    then
;




( Given a document uri, and a location within a source text.  Try to find a word at that location. )
( This search will only return a word, so if the location lands on a comment or string, or other )
( type of literal, the search will fail.  Note that if the location lands on document whitespace )
( nothing will be found in that situation either. )
: ds.scan_for_word  ( uri tk.location -- [tk.token] was_found? )
    variable! uri
    variable! location

    location tk.location.line@ variable! line
    location tk.location.character@ variable! character

    variable document
    variable tokens

    variable size
    variable index
    variable token

    false variable! found_word?

    uri @  ds.find_document
    if
        document !
        document ds.document.token_list@  tokens !
        tokens [].size@@  size !

        begin
            index @  size @  <
        while
            tokens [ index @ ]@@  token !

            token tk.token.is_word?
            if
                line @  token tk.token.location.line@  =
                if
                    character @  token tk.token.location.character@  >=
                    character @  token tk.token.location.character@
                                 token tk.token.contents@ string.size@  +  <
                    &&
                    if
                        true found_word? !
                        break
                    then
                then
            then

            line @  token tk.token.location.line@  <
            if
                break
            then

            index ++!
        repeat
    then

    found_word? @
    if
        token @
        true
    else
        false
    then
;




( Search the global symbol table and attempt to find the word in question. )
: ds.find_symbol  ( word -- [ds.document.symbol] was_found? )
    variable! found_word

    found_word @  ds.symbol_table @  {}?
    if
        ds.symbol_table { found_word @ }@@
        true
    else
        false
    then
;
