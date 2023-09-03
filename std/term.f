
( Some useful words when dealing with the terminal. )
"\027"         constant term.esc  ( Terminal escape character. )
term.esc "[" + constant term.csi  ( Control sequence introducer. )


( Read from the terminal and expect it to be a specific character.  If it isn't a match an )
( exception is thrown. )
: term.expect_key description: "Expect a given key be read from cin, throw an exception otherwise."
    ( expected_key -- )

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
: term.read_num_until description: "Attempt to read a number up until a given character is found."
    ( terminator_char -- read_number )

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


( Get the terminal's current cursor position. )
: term.cursor_position@  ( -- row column ) description: "Read the current cursor position."
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
: term.cursor_column@ description: "Read just the cursor's current column."
    term.cursor_position@

    swap
    drop
;


: term.cursor_left! description: "Move the cursor left a given number of spaces."
    term.csi swap + "D" + term.!
    term.flush
;


: term.cursor_right! description: "Move the cursor right a given number of spaces."
    term.csi swap + "C" + term.!
    term.flush
;


: term.cursor_save description: "Save the current cursor location."
    term.esc "7" + term.!
    term.flush
;


: term.cursor_restore description: "Restore the current cursor location."
    term.esc "8" + term.!
    term.flush
;


( Clear the entire line the cursor is on. )
: term.clear_line  ( -- ) description: "Clear the entire line the cursor is on."
    term.csi "2K\r" + term.!
    term.flush
;
