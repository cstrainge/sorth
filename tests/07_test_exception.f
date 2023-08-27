
: try_test
    "Testing a try!" .cr

    try
        ( Throw a silly exception, the quit statement should never be reached. )
        "What happened?" throw
        exit_failure quit
    catch
        ( We caught the exception, and can display it. )
        "Exception caught!" .cr
        .cr

        ( Let's make sure code outside of this word can also catch exceptions. )
        "Oh no!" throw
    endcatch
;


try
    ( Call the word and wait for the exception.  Again, the quit should never be executed. )
    try_test
    exit_failure quit
catch
    "Second level catch run!" .cr
    .cr
endcatch
