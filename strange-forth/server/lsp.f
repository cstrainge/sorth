
-32700 constant lsp.parse_error
-32600 constant lsp.invalid_request
-32601 constant lsp.method_not_found
-32602 constant lsp.invalid_params
-32603 constant lsp.internal_error

jsonrpc.reserved_error_range_start constant lsp.server_error_start

-32002 constant lsp.server_not_initialized
-32001 constant lsp.unknown_error_code

jsonrpc.reserved_error_range_end constant lsp.server_error_end

-32899 constant lsp.reserved_error_range_start
-32803 constant lsp.request_failed
-32802 constant lsp.server_canceled
-32801 constant lsp.content_modified
-32800 constant lsp.request_cancelled
-32800 constant lsp.reserved_error_range_end



0 constant lsp.text_document_sync_kind.none
1 constant lsp.text_document_sync_kind.full
2 constant lsp.text_document_sync_kind.incremental



: lsp.begin_shutdown
    true lsp.is_shutting_down
;


: lsp.success_response  ( id response_value -- )
    "result" jsonrpc.send_message_response
;


: lsp.failed_response  ( id response_value -- )
    "error" jsonrpc.send_message_response
;


: lsp.process_message  ( message -- )
    variable! message

    message { "method" }@@ variable! method
    variable id

    "id" message @ {}?
    if
        message { "id" }@@ id !
    else
        "" id !
    then

    try
        method @ lsp.message_registrar @ {}?
        if
            message { "params" }@@ lsp.message_registrar { method @ }@@ execute

            id @ "" <>
            if
                if
                    id @ swap lsp.success_response
                else
                    id @ swap lsp.failed_response
                then
            then
        else
            id @

            {}.new

            dup lsp.method_not_found                 swap "code"    swap {}!
            dup "No registration found for message." swap "message" swap {}!

            lsp.failed_response
        then
    catch
        "Message handling exception." .cr
        .cr

        id @

        {}.new

        dup lsp.internal_error                    swap "code"    swap {}!
        dup "Internal error, exception occurred." swap "message" swap {}!

        lsp.failed_response
    endcatch
;



{}.new variable! lsp.message_registrar
false variable! lsp.is_shutting_down




: lsp.on:initialize lsp.message_registrar { "initialize" }!! ;
: lsp.on:initialized lsp.message_registrar { "initialized" }!! ;


: lsp.on:$/set_trace lsp.message_registrar { "$/setTrace" }!! ;


: lsp.on:text_document/did_open lsp.message_registrar { "textDocument/didOpen" }!! ;
: lsp.on:text_document/hover lsp.message_registrar { "textDocument/hover" }!! ;
: lsp.on:text_document/document_highlight lsp.message_registrar { "textDocument/documentHighlight" }!! ;




: lsp.process_message_loop  ( -- )
    begin
        try
            jsonrpc.read_message lsp.process_message
        catch
            "An internal error occurred." .cr
            .cr
        endcatch

        lsp.is_shutting_down @
    until
;
