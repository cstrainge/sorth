
( Implementations of the standard library words {}.to_json and {}.from_json, and their helper )
( words. )




( Convert an array to a json compatible string. )
: json.to_json_array hidden  ( array -- formatted_string )
    variable! array_value
    0 variable! index
    "[ " variable! array_str

    begin
        array_str @ array_value [ index @ ]@@ json.to_json_value + array_str !

        index @ array_value [].size@@ -- <
        if
            array_str @ ", " + array_str !
        then

        index ++!
        index @ array_value [].size@@ >=
    until

    array_str @ " ]" +
;




: json.filter_json_string hidden  ( string -- filtered_string )
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
            "\n" of "\\n" next_char ! endof
            "\t" of "\\t" next_char ! endof
            "\"" of "\\\"" next_char ! endof
        endcase

        new @  next_char @  +  new !

        index ++!
    repeat

    new @
;




( Convert a given value to a json formatted string. )
: json.to_json_value hidden  ( value -- string )
    dup is_value_string?
    if
        "\"" swap json.filter_json_string + "\"" +
    else
        dup is_value_hash_table?
        if
            {}.to_json
        else
            dup is_value_array?
            if
                json.to_json_array
            else
                dup is_value_number?
                dup is_value_boolean?
                ||
                if
                    to_string
                else
                    dup is_value_structure?
                    if
                        #.to_json
                    else
                        drop
                        "Unsupported json value type." throw
                    then
                then
            then
        then
    then
;




: #.to_json  description: "Convert a structure object to a JSON string"
             signature: "structure -- json_string"
    variable! structure
    "{ " variable! new_json

    : json.struct_iterator hidden
        variable! value
        variable! name

        "\"" name @ to_string + "\"" + ": " + value @ json.to_json_value + ", " +
        new_json @ swap + new_json !
    ;

    ` json.struct_iterator structure @ #.iterate

    new_json @ string.size@ 2 >
    if
        2 new_json @ dup string.size@ 2 - swap string.remove new_json !
    then

    new_json @ " }" +
;




: {}.to_json  description: "Convert a hash table into a JSON string."
              signature: "hash_table -- json_string"
    variable! hash
    "{ " variable! new_json

    : json.hash_iterator hidden
        variable! value
        variable! key

        "\"" key @ to_string + "\"" + ": " + value @ json.to_json_value + ", " +
        new_json @ swap + new_json !
    ;

    ` json.hash_iterator hash @ {}.iterate

    new_json @ string.size@ 2 >
    if
        2 new_json @ dup string.size@ 2 - swap string.remove new_json !
    then

    new_json @ " }" +
;




( Keep track of the line/column we are on in the input json. )
# json.location line column ;


( Create a new instance of the structure, properly initialized. )
: json.location.new hidden ( -- new_location )
    json.location.new

    dup 1 json.location.line rot rot #!
    dup 1 json.location.column rot rot #!
;


( Member variable accessors they take a variable index and set/get the value of the field. )
: json.location.line!!    hidden  json.location.line   swap @ #! ;
: json.location.column!!  hidden  json.location.column swap @ #! ;

: json.location.line@@    hidden  json.location.line   swap @ #@ ;
: json.location.column@@  hidden  json.location.column swap @ #@ ;


( Take a character and properly increment the line/column as needed. )
: json.location.inc  hidden  ( character json.location --  )
    variable! location

    "\n" =
    if
        ( We're incrementing lines, so reset colum and increment the line. )
        1 location json.location.column!!
        location json.location.line@@ ++ location json.location.line!!
    else
        ( This isn't a new line, so we're just incrementing the column. )
        location json.location.column@@ ++ location json.location.column!!
    then
;




( String structure used for parsing json.  We use it to keep track of where we are in the string )
( during parsing.  For in a logical line/colum way and directly as in the index into the string )
( variable. )
# json.string location index source ;


( Member variable accessors they take a variable index and set/get the value of the field. )
: json.string.location!!  hidden  json.string.location swap @ #! ;
: json.string.index!!     hidden  json.string.index    swap @ #! ;
: json.string.source!!    hidden  json.string.source   swap @ #! ;

: json.string.location@@  hidden  json.string.location swap @ #@ ;
: json.string.index@@     hidden  json.string.index    swap @ #@ ;
: json.string.source@@    hidden  json.string.source   swap @ #@ ;


( Create a new initialized instance of the parsing structure. )
: json.string.new hidden ( string -- json.string )
    json.string.new variable! new_json

    json.location.new new_json json.string.location!!
    0                 new_json json.string.index!!
                      new_json json.string.source!!

    new_json @
;




( Increment the current location and string positions. )
: json.string.inc hidden ( character json_string_var_index -- )
    over json.string.location@@ json.location.inc
    dup json.string.index@@ ++ swap json.string.index!!
;




( Take a peek at the next character in the stream without advancing the pointer. )
: json.string.peek@ hidden ( json.string_var -- character )
    dup json.string.index@@
    swap json.string.source@@

    string.[]@
;




( Check to see if the pointer is at the end of the string or not. )
: json.string.eos@ hidden ( json.string_var -- is_eos )
    dup json.string.index@@
    swap json.string.source@@ string.size@

    >=
;




( Get a character from the string and advance the pointer. )
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




( Report an error in the json string. )
: json.error hidden  ( message json.string --  )
    @ variable! json_source
    variable! message

    json_source json.string.location@@ variable! location

    "[" location json.location.line@@ + ", " + location json.location.column@@ + "]: " +
    message @ + throw
;




( Skip past any whitespace in the json string. )
: json.skip_whitespace hidden  ( json.string -- )
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




( Expect the next character in the string is the one given.  Throw an error if not. )
: json.expect_char hidden ( char json.string -- )
    @ variable! json_source
    variable! expected
    variable found

    json_source json.string.next@ dup found !
    expected @ <>
    if
        "Expected the character '" expected @ + "' in json string found, '" + found @ + "'." +
        json_source json.error
    then
;




( Expect a specific substring from the json string.  Throw an error if it's missing. )
: json.expect_string hidden ( expected_str json_source -- )
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




( Read a string literal from the json source. )
: json.read_string hidden  ( json.string -- string_value )
    @ variable! json_source
    "" variable! new_string
    variable next_char

    json_source json.skip_whitespace
    "\"" json_source json.expect_char

    begin
        json_source json.string.eos@ '
        json_source json.string.peek@ "\"" <> &&
    while
        json_source json.string.next@ next_char !

        next_char @ dup "\\" =
        if
            json_source json.string.next@ dup
            case
                "n" of drop "\n" next_char ! endof
                "r" of drop "\r" next_char ! endof
                "t" of drop "\t" next_char ! endof

                next_char !
            endcase
        then

        new_string @ next_char @ + new_string !
    repeat

    "\"" json_source json.expect_char

    new_string @
;




( Read an array of values from the json source. )
: json.read_array hidden  ( json.string -- array_value )
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




( Is the given character considered numeric? )
: json.is_numeric? hidden  ( character -- is_numeric? )
    variable! next_char

    next_char @ "0" >=
    next_char @ "9" <= &&

    next_char @ "." =
    next_char @ "-" =  ||

    ||
;




( Read a numeric value from the json string. )
: json.read_number hidden  ( json.string -- number )
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




( Read a literal value from the json input. )
: json.read_value hidden  ( json.string -- value )
    @ variable! json_source
    variable new_value

    json_source json.skip_whitespace

    json_source json.string.eos@
    if
        "Unexpected end of json string." json_source json.error
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
            "Unexpected json value type." json_source json.error
        then
    endcase

    new_value @
;




( Read a hash value from the json string in key/value pairs. )
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




: {}.from_json
    description: "Convert a JSON formatted string into a hash table."
    signature: "json_string -- hash_table"

    json.string.new variable! json_source

    json_source json.skip_whitespace
    json_source json.string.peek@

    "{" <>
    if
        "Expected json object." json_source json.error
    then

    json_source json.read_hash
;
