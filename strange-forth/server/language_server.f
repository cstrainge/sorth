
( Get the path to the socket from the command line arguments, then properly connect to it.)
"--pipe=" string.size@ 0 args [ 0 ]@ string.remove constant socket_path

socket_path socket.connect variable! server_fd



: dump_stack
    "-----" .cr
    .s
    "-----" .cr
;



"jsonrpc.f" include
"lsp.f" include
"tokenizer.f" include
"document_store.f" include



: ls.handle:initialize ( message_params -- response_data was_successful )
    ( TODO: Read the message params. )

    "ls.handle:initialize" .cr

    drop

    {
        "capabilities" -> {
            "textDocumentSync" -> lsp.text_document_sync_kind.full ,
            "hoverProvider" -> true ,
            "signatureHelpProvider" -> true ,
            "definitionProvider" -> true ,
            "typeDefinitionProvider" -> true ,
            "documentHighlightProvider" -> true
        } ,
        "serverInfo" -> {
            "name" -> "Strange Forth" ,
            "version" -> "0.0.0"
        }
    }

    true
;


: ls.handle:initialized
    "I've been initialized!" .cr
;



: ls.handle:text_document/did_open
    "A document has been opened."
;


: ls.handle:text_document/document_highlight
    "Document highlight request".

    { }
    true
;


: ls.handle:text_document/hover
    "A document hovered."

    { }
    true
;



` ls.handle:initialize lsp.on:initialize
` ls.handle:initialized lsp.on:initialized

` ls.handle:text_document/did_open lsp.on:text_document/did_open
` ls.handle:text_document/document_highlight lsp.on:text_document/document_highlight
` ls.handle:text_document/hover lsp.on:text_document/hover


lsp.process_message_loop
