
: loop_until ( -- )
    10 begin
        "Looping until: " .
        -- dup .cr

        dup 0<=
    until

    drop
;


: loop_while ( -- )
    10 begin -- dup 0>=
    while
        "Looping while: " .
        dup .cr
    repeat

    drop
;


loop_until
cr
loop_while
