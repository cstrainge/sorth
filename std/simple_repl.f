
( Define a user prompt for the REPL. )
: prompt description: "Prints the user input prompt in the REPL."
    ">>" .
;


( This gets called automatically when the repl exits. )
: repl.exit_handler hidden "ok" .cr ;


( Implementation of the language's REPL. )
: repl description: "Sorth's REPL: read, evaluate, and print loop."
    "Strange Forth REPL." .cr
    cr
    "Enter quit, q, or exit to quit the REPL." .cr
    cr

    at_exit repl.exit_handler

    begin
        ( Loop forever, because when the user enters the quit command the interpreter will exit )
        ( gracefully on it's own. )
        true
    while
        try
            ( Always make sure we get the newest version of the prompt.  That way the user can )
            ( change it at runtime. )
            cr "prompt" execute

            ( Get the text from the user and execute it.  We are just using a really simple )
            ( implementation of readline for now. )
            term.readline
            code.execute_source

            ( If we got here, everything is fine. )
            "ok" .cr
        catch
            ( Something in the user code failed, display the error and try again. )
            cr .cr
        endcatch
    repeat
;
