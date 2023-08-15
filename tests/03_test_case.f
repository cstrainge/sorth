
( Just a really basic test of the case statement. )
: hello_something
    "Hello" .

    case
        1 of "world."        endof
        2 of "solar system." endof
        3 of "universe!"     endof

        "everything else!"
    endcase

    .cr
;


( Ok, try out all the options... )
1 hello_something
2 hello_something
3 hello_something
1024 hello_something
