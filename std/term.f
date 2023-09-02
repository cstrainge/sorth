
( Some useful words when dealing with the terminal. )
"\027"         constant term.esc  ( Terminal escape character. )
term.esc "[" + constant term.csi  ( Control sequence introducer. )


( Read from the terminal and expect it to be a specific character.  If it isn't a match an )
( exception is thrown. )
: term.expect_key  ( extpected_key -- )
    variable! expected
    term.key variable! got

    expected @ got @ <>
    if
        expected @ term.is_printable? '
        if
            expected @ hex expected !
        then

        got @ term.is_printable? '
        if
            got @ hex got !
        then

        "Did not get expected character, " expected @ + ", received " + got @ + "." + throw
    then
;


( Read numeric characters from the terminal until an expected terminator character is found. )
: term.read_num_until  ( terminator_char -- read_numberr )
    variable! until_char
    "" variable! read_str

    begin
        term.key

        dup until_char @ <>
        if
            dup read_str @ swap + read_str !
        then

        until_char @ =
    until

    read_str @ string.to_number
;


( Get the terminal's currrent cursor position. )
: term.cursor_position@  ( -- row column )
    variable pos_r
    variable pos_c

    term.csi "6n" + term.! term.flush

    ( Expecting csi r ; c R )

    term.esc term.expect_key
    "[" term.expect_key
    ";" term.read_num_until pos_r !
    "R" term.read_num_until pos_c !

    pos_r @
    pos_c @
;

( Get the current column the cursor is in. )
: term.cursor_column@
    term.cursor_position@

    swap
    drop
;


: term.cursor_left!
    term.csi swap + "D" + term.!
    term.flush
;


: term.cursor_right!
    term.csi swap + "C" + term.!
    term.flush
;


: term.cursor_save
    term.esc "7" + term.!
    term.flush
;


: term.cursor_restore
    term.esc "8" + term.!
    term.flush
;


( Clear the entire line the cursor is on. )
: term.clear_line  ( -- )
    term.csi "2K\r" + term.!
    term.flush
;
