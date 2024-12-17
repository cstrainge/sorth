
( Define a user prompt for the REPL. )
: prompt description: "Prints the user input prompt in the REPL."
    ">> " .
;


( Ways to exit the repl. )
false variable! repl.is_quitting?


: quit description: "Exit the repl."
       signature: " -- "
    true repl.is_quitting? !
;


: q  description: "Exit the repl."
     signature: " -- "
    quit
;


: exit description: "Exit the repl."
       signature: " -- "
    quit
;


( Implementation of the language's REPL. )
: repl description: "Sorth's REPL: read, evaluate, and print loop."
       signature: " -- "

    ( Print the welcome banner along with information about the interpreter. )
    sorth.version
    sorth.compiler
    sorth.execution-mode
    "*
       Strange Forth REPL.

       Version: {}
       Compiled with: {}
       Execution mode: {}

       Enter quit, q, or exit to quit the REPL.
       Enter .w to show defined words.
       Enter show_word <word_name> to list detailed information about a word.

    *"
    string.format .cr

    begin
        repl.is_quitting? @ '
    while
        try
            ( Always make sure we get the newest version of the prompt.  That way the user can )
            ( change it at runtime. )
            cr "prompt" execute

            ( Get the text from the user and execute it.  We are just using a really simple )
            ( implementation of readline for now. )
            term.readline "<repl>" code.execute_source

            ( If we got here, everything is fine. )
            "ok" .cr
        catch
            ( Something in the user code failed, display the error and try again. )
            cr .cr
        endcatch
    repeat
;
