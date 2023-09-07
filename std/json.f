
: to_json_array hidden
    variable! array_value
    0 variable! index
    "[ " variable! array_str

    begin
        array_str @ array_value [ index @ ]@@ to_json_value + array_str !

        index @ array_value [].size@@ -- <
        if
            array_str @ ", " + array_str !
        then

        index ++!
        index @ array_value [].size@@ >=
    until

    array_str @ " ]" +
;


: to_json_value hidden
    dup is_value_string?
    if
        "\"" swap + "\"" +
    else
        dup is_value_hash_table?
        if
            {}.to_json
        else
            dup is_value_array?
            if
                to_json_array
            else
                dup is_value_number?
                dup is_value_boolean?
                ||
                if
                    to_string
                else
                    drop
                    "Unsupported json value type." throw
                then
            then
        then
    then
;


: {}.to_json description: "Convert a hash table into a JSON string."
    variable! hash
    "{ " variable! new_json

    : iterator hidden
        variable! value
        variable! key

        "\"" key @ to_string + "\"" + ": " + value @ to_json_value + ", " +
        new_json @ swap + new_json !
    ;

    ` iterator hash @ {}.iterate

    new_json @ string.size@ 2 >
    if
        2 new_json @ dup string.size@ 2 - swap string.remove new_json !
    then

    new_json @ " }" +
;


( Keep track of the line/column we are on in the input json. )
# json.location line column ;

: json.location.new
    json.location.new

    dup 1 json.location.line rot rot #!
    dup 1 json.location.column rot rot #!
;


: json.location.line!!    hidden  json.location.line   swap @ #! ;
: json.location.column!!  hidden  json.location.column swap @ #! ;

: json.location.line@@    hidden  json.location.line   swap @ #@ ;
: json.location.column@@  hidden  json.location.column swap @ #@ ;


: json.location.inc  hidden  ( character json.location --  )
    variable! location

    "\n" =
    if
        ( We're incrmenting lines, so reset colum and increment the line. )
        1 location json.location.column!!
        location json.location.line@@ ++ location json.location.line!!
    else
        ( This isn't a new line, so we're just incrementing the column. )
        location json.location.column@@ ++ location json.location.column!!
    then
;




( String structure used for parsing json.  We use it to keep track of where we are in the string )
( during parsing.  For in a logical line/colum way and directly as in the index into a string )
( variable. )
# json.string location index source ;


: json.string.location!!  hidden  json.string.location swap @ #! ;
: json.string.index!!     hidden  json.string.index    swap @ #! ;
: json.string.source!!    hidden  json.string.source   swap @ #! ;

: json.string.location@@  hidden  json.string.location swap @ #@ ;
: json.string.index@@     hidden  json.string.index    swap @ #@ ;
: json.string.source@@    hidden  json.string.source   swap @ #@ ;


: json.string.new hidden ( string -- json.string )
    json.string.new variable! new_json

    json.location.new new_json json.string.location!!
    0                 new_json json.string.index!!
                      new_json json.string.source!!

    new_json @
;

: json.string.inc hidden ( json_string_var_index -- )
    over json.string.location@@ json.location.inc
    dup json.string.index@@ ++ swap json.string.index!!
;

: json.string.peek@ hidden ( json.string_var -- character )
    dup json.string.index@@
    swap json.string.source@@

    string.[]@
;

: json.string.eos@ hidden ( json.string_var -- is_eos )
    dup json.string.index@@
    swap json.string.source@@ string.size@

    >=
;

: json.string.next@ hidden ( json.string_var -- character )
    dup json.string.eos@ '
    if
        dup json.string.peek@
        over swap json.string.inc
    else
        drop
        " "
    then
;


: json.skip_whitespace hidden
    @ variable! json_source
    variable next

    begin
        json_source json.string.peek@ next !

        next @ "\n" = next @ "\t" = || next @ " "  = ||
        json_source json.string.eos@ ' &&
    while
        json_source json.string.next@ drop
    repeat
;


: json.expect_char hidden ( char json.string -- )
    @ variable! json_string
    variable! expected
    variable found

    json_string json.string.next@ dup found !
    expected @ <>
    if
        "Expected the character '" expected @ + "' in json string found, '" + found @ + "'." + throw
    then
;


: json.expect_string hidden ( expected_str json_source == )
    @ variable! json_source
    variable! expected

    expected string.size@@ variable! size
    0 variable! index

    begin
        index @ expected @ string.[]@ json_source json.expect_char

        index ++!
        index @ size @ >=
    until
;


: json.read_string hidden
    @ variable! json_source
    "" variable! new_string

    json_source json.skip_whitespace
    "\"" json_source json.expect_char

    begin
        json_source json.string.eos@ '
        json_source json.string.peek@ "\"" <> &&
    while
        new_string @ json_source json.string.next@ + new_string !
    repeat

    "\"" json_source json.expect_char

    new_string @
;


: json.read_array hidden
    @ variable! json_source
    0 [].new variable! new_array
    0 variable! index

    json_source json.skip_whitespace
    "[" json_source json.expect_char

    begin
        json_source json.skip_whitespace

        json_source json.string.eos@ '
        json_source json.string.peek@ "]" <> &&
    while
        index @ ++ new_array [].size!!
        json_source json.read_value new_array [ index @ ]!!

        index ++!

        json_source json.skip_whitespace
        json_source json.string.peek@ "," <>
        if
            break
        then

        json_source json.string.next@
        drop
    repeat

    json_source json.skip_whitespace
    "]" json_source json.expect_char

    new_array @
;


: json.is_numeric? hidden
    variable! next_char

    next_char @ "0" >=
    next_char @ "9" <= &&

    next_char @ "." =
    next_char @ "-" =  ||

    ||
;


: json.read_number hidden
    @ variable! json_source
    "" variable! new_number_text

    begin
        json_source json.string.eos@ '
        json_source json.string.peek@ json.is_numeric?
    while
        new_number_text @ json_source json.string.next@ + new_number_text !
    repeat

    new_number_text @ string.to_number
;


: json.read_value hidden
    @ variable! json_source
    variable new_value

    json_source json.skip_whitespace

    json_source json.string.eos@
    if
        "Unexpected end of json string." throw
    then

    json_source json.string.peek@
    case
        "t"  of "true"  json_source json.expect_string   true new_value !  endof
        "f"  of "false" json_source json.expect_string  false new_value !  endof
        "["  of         json_source json.read_array           new_value !  endof
        "{"  of         json_source json.read_hash            new_value !  endof
        "\"" of         json_source json.read_string          new_value !  endof

        json_source json.string.peek@ json.is_numeric?
        if
            json_source json.read_number new_value !
        else
            "Unexpected json value type." throw
        then
    endcase

    new_value @
;


: json.read_hash hidden
    @ variable! json_source
    {}.new variable! new_hash

    variable key

    "{" json_source json.expect_char

    begin
        json_source json.skip_whitespace

        json_source json.string.eos@ '
        json_source json.string.peek@ "}" <> &&
    while
        json_source json.read_string key !

        json_source json.skip_whitespace
        ":" json_source json.expect_char

        json_source json.read_value new_hash { key @ }!!

        json_source json.skip_whitespace
        json_source json.string.peek@ "," <>
        if
            break
        then

        json_source json.string.next@
        drop
    repeat

    json_source json.skip_whitespace
    "}" json_source json.expect_char

    new_hash @
;


: {}.from_json ( string -- hash_table )
    description: "Convert a JSON formatted string into a hash table."

    json.string.new variable! json_source

    json_source json.skip_whitespace
    json_source json.string.peek@

    "{" <>
    if
        "Expected json object." throw
    then

    json_source json.read_hash
;
