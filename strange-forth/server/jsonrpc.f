
-32099 constant jsonrpc.reserved_error_range_start
-32000 constant jsonrpc.reserved_error_range_end


: jsonrpc.read_char  ( -- character )
    server_fd @ file.char@
;


: jsonrpc.read_string  ( byte_count -- string )
    server_fd @ file.string@
;


: jsonrpc.write_string  ( string -- )
    server_fd @ file.!
;


: jsonrpc.read_header_line  ( -- key value )
    "" variable! key
    "" variable! value

    false variable! found_colon
    false variable! done

    begin
        jsonrpc.read_char

        dup "\r" =
        if
            drop

            jsonrpc.read_char "\n" <>
            if
                "Malformed line." throw
            then

            true done !
        else
            dup ":" =
            if
                true found_colon !
                drop
            else
                found_colon @
                if
                    value @ swap + value !
                else
                    key @ swap + key !
                then
            then
        then

        done @
    until

    value @
    key @
;


# jsonrpc.header content_type content_length ;

: jsonrpc.header.content_type!!      jsonrpc.header.content_type   swap @ #! ;
: jsonrpc.header.content_length!!    jsonrpc.header.content_length swap @ #! ;

: jsonrpc.header.content_type@@      jsonrpc.header.content_type   swap @ #@ ;
: jsonrpc.header.content_length@@    jsonrpc.header.content_length swap @ #@ ;


: jsonrpc.header.new  ( -- new_header )
    jsonrpc.header.new variable! header

    "application/vscode-jsonrpc; charset=utf-8" header jsonrpc.header.content_type!!
    0                                           header jsonrpc.header.content_length!!

    header @
;


: jsonrpc.read_headers  ( -- read_header )
    jsonrpc.header.new variable! header
    variable header_field

    begin
        jsonrpc.read_header_line header_field !

        header_field @
        case
            "Content-Type" of
                header jsonrpc.header.content_type!!
                endof

            "Content-Length" of
                string.to_number header jsonrpc.header.content_length!!
                break
                endof
        endcase

        false
    until

    jsonrpc.read_char "\r" =
    jsonrpc.read_char "\n" =
    && '
    if
        "Expected \\r and \\n characters to finish json-rpc header." throw
    then

    header @
;


: jsonrpc.read_message  ( -- message_hash )
    "Waiting for incoming message." log_message

    jsonrpc.read_headers variable! header
    variable json_body

    header jsonrpc.header.content_length@@ jsonrpc.read_string json_body !

    "Read Message: " header @ to_string + log_message

    json_body @ .cr

    json_body @ {}.from_json

    dup "id" swap {}?
    if
        dup { "id" }@     "ID:     " swap + .cr
    then

    dup "method" swap {}?
    if
        dup { "method" }@ "method: " swap + .cr
    then
;


: jsonrpc.send_message_response  ( id response_value response_key -- )
    variable! response_key
    {}.to_json variable! response_value
    to_string variable! id

    "{ \"jsonrpc\": \"2.0\", "

    id @ "" <>
    if
        "\"id\": " + id @ + ", " +
    then

    "\"" + response_key @ + "\": " + response_value @ + " }" +

    variable! message

    "Content-Length: " message @ string.size@ + "\r\n\r\n" + message @ + message !

    "---[ Responding ]------" .cr
    message @ .cr
    message @ jsonrpc.write_string
    "-----------------------" .cr
;
