
( Low level implementation of the JSON-RPC protocol. This is a very lightweight implementation and )
( only provides what the language server currently requires to operate. )




( Base error constants. )
-32099 constant jsonrpc.reserved_error_range_start
-32000 constant jsonrpc.reserved_error_range_end




( Read a character from the incoming message stream.  Block if there is nothing to be read. )
: jsonrpc.read_char  ( -- character )
    server_fd @ file.char@
;




( Read a string of byte_count characters from the incoming message stream.  This can block. )
: jsonrpc.read_string  ( byte_count -- string )
    server_fd @ file.string@
;




( Write a string to the outgoing message stream.  This can potentially block. )
: jsonrpc.write_string  ( string -- )
    server_fd @ file.!
;




( Read a line from the JSON RPC message header. )
: jsonrpc.read_header_line  ( -- value key )
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




( Structure that represents a JSON-RPC message header.  The content_type is ignored in this )
( implementation of the protocol. )
# jsonrpc.header
    content_type    ( Mostly just ignored, if received it should be the following: )
                    ( "application/vscode-jsonrpc; charset=utf-8" )
    content_length  ( The length of the message body. )
;


: jsonrpc.header.content_type!!      jsonrpc.header.content_type   swap @ #! ;
: jsonrpc.header.content_length!!    jsonrpc.header.content_length swap @ #! ;

: jsonrpc.header.content_type@@      jsonrpc.header.content_type   swap @ #@ ;
: jsonrpc.header.content_length@@    jsonrpc.header.content_length swap @ #@ ;




( Override the default constructor and setup reasonable default values. )
: jsonrpc.header.new  ( -- new_header )
    jsonrpc.header.new variable! header

    "application/vscode-jsonrpc; charset=utf-8" header jsonrpc.header.content_type!!
    0                                           header jsonrpc.header.content_length!!

    header @
;




( Attempt to read a JSON-RPC message header from the incoming message stream.  This word can block )
( while waiting for input. )
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




( Read a full message from the JSON RPC client.  Then convert the json contents into a hash table. )
: jsonrpc.read_message  ( -- message_hash )
    "Waiting for incoming message." .cr

    jsonrpc.read_headers variable! header
    variable json_body

    header jsonrpc.header.content_length@@ jsonrpc.read_string json_body !

    "Read Message: " header @ to_string + .cr

    json_body @ .cr

    json_body @ {}.from_json


    ( For now log the details of this message to our server log. As the server stabilizes this )
    ( should be removed or replaced with a bitter logging system. )
    dup "id" swap {}?
    if
        dup { "id" }@     "ID:     " swap + .cr
    then

    dup "method" swap {}?
    if
        dup { "method" }@ "method: " swap + .cr
    then
;




( Send a response to a request message that previously came in.  The ID is to help the recipient )
( properly correlate the response with the original query. )
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

    ( Log the message and send it along to the client.  As the server stabilizes this logging )
    ( should either be made more robust or just removed. )
    "---[ Responding ]------" .cr
    message @ .cr
    message @ jsonrpc.write_string
    "-----------------------" .cr
;




( Send an unsolicited message from the server to the client. )
: jsonrpc.send_message ( id method params -- )
    variable! params
    variable! method
    variable! id

    "{ \"jsonrpc\": \"2.0\", "

    id @ "" <>
    if
        "\"id\": " + id @ + ", " +
    then

    "\"method\": \"" + method @ + "\", \"params\": " + params @ {}.to_json + " }" +
    variable! message

    "Content-Length: " message @ string.size@ + "\r\n\r\n" + message @ + message !

    ( Log the message and send it along to the client.  As the server stabilizes this logging )
    ( should either be made more robust or just removed. )
    "---[ Transmitting ]----" .cr
    message @ .cr
    message @ jsonrpc.write_string
    "-----------------------" .cr
;
