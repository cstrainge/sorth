
( Core file of the Strange Forth language server.  This file acts as the program's main. All of )
( writes to standard out are currently captured by the launch script and shunted to the file )
( sorth_lsp_log.txt.  It is planned as the language server stabilizes that this level of logging )
( will be removed. )
"Server started." .cr




( Get a copy of the dictionary before we've added any words.  This way we have a consistent set )
( of words that represent the standard library. )
words.get{} constant ls.standard_words

"Standard words gathered." .cr




( Get the path to the socket from the command line arguments, then properly connect to it. )
"Opening LSP connection to client.".cr

"--pipe=" string.size@ 0 args [ 0 ]@ string.remove constant socket_path
socket_path socket.connect variable! server_fd




( Include the rest of the language server. )
"jsonrpc.f" include
"lsp.f" include
"tokenizer.f" include
"document_store.f" include



( Make sure to add the standard symbols to the symbol table. )
ls.standard_words ds.add_std_symbols




( Handle the initialize message sent from the language client. We don't examine the parameters )
( sent yet and simply respond with a simple set of capabilities. )
( This word should be improved to configure and better report the server capabilities. )
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




( The init process has completed. )
: ls.handle:initialized
    "Client/server connection has been successfully initialized." lsp.log_message
;




( We have been requested to begin shutdown. )
: ls.handle:shutdown
    "The server is shutting down." lsp.log_message

    lsp.begin_shutdown

    {
    }
    true
;




( Ok, now the client wants us to exit for real. )
: ls.handle:exit
    "Final exit notification received." lsp.log_message

    lsp.handle_exit
;




( Set/activate/deactivate the language server's trace level.  We don't really do anything with )
( this flag at this moment. )
: ls.handle:$/set_trace
    variable! params
    params { "value" }@@ variable! trace_value

    "Trace set to: " trace_value @ + lsp.log_message
;




( Handle the fact that a document has been opened in the client.  What we do here is store the )
( full text of the document in our document store and process it for symbols and any other useful )
( information we can extract.  Right now there's a perf issue in string handling so the initial )
( tokenization can take multiple seconds on larger documents.  A language change is being planned )
( to address this. )
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




( Handle a document highlight request. We don't do anything with the request right now. )
: ls.handle:text_document/document_highlight
    "Document highlight request" lsp.log_message
    drop

    { }
    true
;




( Handle a client's hover request.  We take the line and character location and scan our )
( document's token list and try to find a word token that the requested location lands on.  If )
( a word token is found, we try to look up a symbol for that word and return what information we )
( find to the language client. )
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




( Document definition request.  We don't do anything with this at this time. )
: ls.handle:text_document/definition
    variable! params

    params { "textDocument" }@@ { "uri" }@   variable! uri
    params { "position" }@@ { "line" }@      variable! line
    params { "position" }@@ { "character" }@ variable! character

    { }
    true
;





( Register our handlers with the LSP message processor, )
` ls.handle:initialize lsp.on:initialize
` ls.handle:initialized lsp.on:initialized
` ls.handle:shutdown lsp.on:shutdown
` ls.handle:exit lsp.on:exit

` ls.handle:$/set_trace lsp.on:$/set_trace

` ls.handle:text_document/did_open lsp.on:text_document/did_open
` ls.handle:text_document/document_highlight lsp.on:text_document/document_highlight
` ls.handle:text_document/hover lsp.on:text_document/hover
` ls.handle:text_document/definition lsp.on:text_document/definition




( All of our init has completed, so run the main message loop and wait for incoming requests. )
" Handlers registered, starting main loop." .cr

lsp.process_message_loop
