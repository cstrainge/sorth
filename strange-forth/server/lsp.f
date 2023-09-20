
( Implementation of the language server protocol, server side.  This is still a pretty early )
( implementation and not all messages and constants are supported yet. )


( Base LSP error codes. )
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



( What kind of document sync do we support. )
0 constant lsp.text_document_sync_kind.none
1 constant lsp.text_document_sync_kind.full
2 constant lsp.text_document_sync_kind.incremental


( Types of log messages supported by the protocol. )
1 constant lsp.log_message.type.error
2 constant lsp.log_message.type.warning
3 constant lsp.log_message.type.info
4 constant lsp.log_message.type.log




( Start the server shutdown procedures. )
: lsp.begin_shutdown  ( -- )
    true lsp.is_shutting_down !
;




( We're received the final exit call, exit from the message loop now. )
: lsp.handle_exit
    true lsp.should_exit_now !
;




( Transmit a successful response to an incoming query. )
: lsp.success_response  ( id response_value -- )
    "result" jsonrpc.send_message_response
;




( Transmit a query failed response back to the client as response to it's query. )
: lsp.failed_response  ( id response_value -- )
    "error" jsonrpc.send_message_response
;




( Log a message from the server to the client. )
: lsp.log_message  ( message -- )
    variable! message

    ""

    "window/logMessage"

    {
        "messageType" -> lsp.log_message.type.log ,
        "message" -> message @
    }

    jsonrpc.send_message
;




( Handle an incoming message from the LSP client.  It is at this point that we look up a handler )
( for the message in our handler table.  If a handler is found, the handler is called.  Otherwise )
( an error is returned.  An error is also returned if an exception is thrown during the processing )
( of the message. )
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
        ( TODO: Respond with lsp.invalid_request if the server is shutting down. )
        method @ lsp.message_registrar @ {}?
        if
            "params" message @ {}?
            if
                message { "params" }@@
            then

            lsp.message_registrar { method @ }@@ execute

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

            {
                "code" -> lsp.method_not_found ,
                "message" -> "No registration found for message."
            }

            lsp.failed_response
        then
    catch
        "Message handling exception." .cr
        .cr

        id @

        {
            "code" -> lsp.internal_error ,
            "message" -> "Internal error, exception occurred."
        }

        lsp.failed_response
    endcatch
;




( LSP server internal state. )
{}.new variable! lsp.message_registrar  ( Registry of message handlers. )
false variable! lsp.is_shutting_down    ( Are we starting to shut down? )
false variable! lsp.should_exit_now     ( Shutdown prep is done, exit the message loop now. )




( Registration handlers.  Handy methods to keep the protocol message string names in one place. )
: lsp.on:initialize lsp.message_registrar { "initialize" }!! ;
: lsp.on:initialized lsp.message_registrar { "initialized" }!! ;
: lsp.on:shutdown lsp.message_registrar { "shutdown" }!! ;
: lsp.on:exit lsp.message_registrar { "exit" }!! ;


: lsp.on:$/set_trace lsp.message_registrar { "$/setTrace" }!! ;


: lsp.on:text_document/did_open lsp.message_registrar { "textDocument/didOpen" }!! ;
: lsp.on:text_document/hover lsp.message_registrar { "textDocument/hover" }!! ;
: lsp.on:text_document/document_highlight lsp.message_registrar { "textDocument/documentHighlight" }!! ;
: lsp.on:text_document/definition lsp.message_registrar { "textDocument/definition" }!! ;




( Core message processing loop.  Wait for incoming JSON-RPC messages and attempt to dispatch them. )
( This word does not return until lsp.handle_exit is called. )
: lsp.process_message_loop  ( -- )
    begin
        try
            jsonrpc.read_message lsp.process_message
        catch
            "An internal error occurred." .cr
            .cr
        endcatch

        lsp.should_exit_now @
    until
;
