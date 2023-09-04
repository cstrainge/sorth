
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

    new_json @ string.length 2 >
    if
        2 new_json @ dup string.length 2 - swap string.remove new_json !
    then

    new_json @ " }" +
;


: {}.from_json description: "Convert a JSON string into a hash table."
    "Unimplemented." throw
;
