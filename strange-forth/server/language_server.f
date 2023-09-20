
"Server started." .cr



( Get a copy of the dictionary before we've added any words.  This way we have a consistent set )
( of words that represent the standard library. )
words.get{} constant ls.standard_words

"Standard words gathered." .cr




( Get the path to the socket from the command line arguments, then properly connect to it. )
"Opening LSP connection to client.".cr

"--pipe=" string.size@ 0 args [ 0 ]@ string.remove constant socket_path
socket_path socket.connect variable! server_fd




"jsonrpc.f" include
"lsp.f" include
"tokenizer.f" include
"document_store.f" include



( Make sure to add the standard symbols to the symbol table. )
ls.standard_words ds.add_std_symbols




: ls.handle:initialize ( message_params -- response_data was_successful )
    ( TODO: Read the message params. )

    "Server received initialize message." lsp.log_message

    drop

    {
        "capabilities" -> {
            "textDocumentSync" -> lsp.text_document_sync_kind.full ,
            "hoverProvider" -> true ,
            "signatureHelpProvider" -> true ,
            "definitionProvider" -> true ,
            "typeDefinitionProvider" -> true
        } ,
        "serverInfo" -> {
            "name" -> "Strange Forth" ,
            "version" -> "0.0.0"
        }
    }

    true
;




: ls.handle:initialized
    "Client/server connection has been successfully initialized." lsp.log_message
;




: ls.handle:shutdown
    "The server is shutting down." lsp.log_message

    lsp.begin_shutdown

    {
    }
    true
;




: ls.handle:exit
    "Final exit notification received." lsp.log_message

    lsp.handle_exit
;




: ls.handle:$/set_trace
    variable! params
    params { "value" }@@ variable! trace_value

    "Trace set to: " trace_value @ + lsp.log_message
;




: ls.handle:text_document/did_open
    variable! params

    params { "textDocument" }@@ { "uri" }@ variable! uri
    params { "textDocument" }@@ { "version" }@ variable! version
    params { "textDocument" }@@ { "text" }@ variable! document

    "A document has been opened." lsp.log_message
    "Uri:     " uri @ +           lsp.log_message
    "Version: " version @ +       lsp.log_message

    uri @ version @ document @ ds.insert_document

    "Document processing complete." lsp.log_message
;




: ls.handle:text_document/document_highlight
    "Document highlight request" lsp.log_message
    drop

    { }
    true
;




: ls.handle:text_document/hover
    variable! params

    params { "position" }@@ { "line" }@ variable! line
    params { "position" }@@ { "character" }@ variable! character
    params { "textDocument" }@@ { "uri" }@ variable! uri

    "Handle document hover request."           lsp.log_message
    "uri:      " uri @ +                       lsp.log_message
    "Position: " line @ + ", " + character @ + lsp.log_message

    ( Documents look for file, and scan for the word at the location. )
    ( Take that word and look it up in our dictionary. )

    variable found_word
    variable symbol
    variable description

    uri @ #.new tk.location {
        line -> line @ ,
        character -> character @
    }
    uri @ ds.scan_for_word
    if
        found_word !

        "--Word was found: " found_word tk.token.contents@ + "--" + lsp.log_message

        found_word tk.token.contents@  ds.find_symbol
        if
            symbol !

            "--Symbol was found--" lsp.log_message

            "### " found_word tk.token.contents@ + "\\n\\n" +

            symbol ds.document.symbol.description@  ""  <>
            if
                "__Description:__ " + symbol ds.document.symbol.description@ + "\\n\\n" +
            then

            symbol ds.document.symbol.signature@  ""  <>
            if
                "__Signature:__\\n" + symbol ds.document.symbol.signature@ + "\\n" +
            then

            description !

            {
                "contents" -> {
                    "kind" -> "markdown" ,
                    "value" -> description @
                } ,
                "range" -> {
                    "start" -> {
                        "line" -> found_word tk.token.location.line@ ,
                        "character" -> found_word tk.token.location.character@
                    } ,
                    "end" -> {
                        "line" -> found_word tk.token.location.line@ ,
                        "character" -> found_word tk.token.location.character@
                                       found_word tk.token.contents@  string.size@  +
                    }
                }
            }
            true
        else
            "--Symbol was not found--" lsp.log_message
            {}.new
            true
        then
    else
        "--Word was not found--" lsp.log_message
        {}.new
        true
    then
;




: ls.handle:text_document/definition
    variable! params

    params { "textDocument" }@@ { "uri" }@   variable! uri
    params { "position" }@@ { "line" }@      variable! line
    params { "position" }@@ { "character" }@ variable! character

    { }
    true
;






` ls.handle:initialize lsp.on:initialize
` ls.handle:initialized lsp.on:initialized
` ls.handle:shutdown lsp.on:shutdown
` ls.handle:exit lsp.on:exit

` ls.handle:$/set_trace lsp.on:$/set_trace

` ls.handle:text_document/did_open lsp.on:text_document/did_open
` ls.handle:text_document/document_highlight lsp.on:text_document/document_highlight
` ls.handle:text_document/hover lsp.on:text_document/hover
` ls.handle:text_document/definition lsp.on:text_document/definition

" Handlers registered, starting main loop." .cr


lsp.process_message_loop
