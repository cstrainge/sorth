
1000 constant repl.history_size
user.home "/.sorth_history.txt" + constant repl.history_path

repl.history_size [].new variable! repl.history

0 variable! repl.history_index
0 variable! repl.history_count


: repl.increment_history_index
    dup @ ++

    dup
    repl.history_size >=
    if
        drop
        0
    then

    swap !
;


: repl.decrement_history_index
    dup @
    --

    dup
    0<
    if
        drop
        repl.history_size --
    then

    swap !
;


: repl.increment_history_count
    repl.history_count @ repl.history_size <
    if
        repl.history_count ++!
    then
;


: repl.insert_history
    dup

    "" <>
    if
        repl.history_index @ repl.history []!!

        repl.history_index repl.increment_history_index
        repl.increment_history_count
    else
        drop
    then
;


: repl.load_history
    -1 variable! fd

    repl.history_path file.r/o file.open fd !

    begin
        fd @ file.is_eof? false =
    while
        fd @ file.line@ repl.insert_history
    repeat

    fd @ file.close
;


: repl.save_history
    -1 variable! fd

    repl.history_path file.w/o file.create fd !

    repl.history_index repl.decrement_history_index

    repl.history_count @ --
    begin
        dup 0>=
    while
        repl.history_index @ repl.history []@@ fd @ file.line!
        repl.history_index repl.decrement_history_index
        --
    repeat
    drop

    fd @ file.close
;


: repl.redraw_line
    term.clear_line
    "\r" term.!

    "prompt" execute

    term.!
    term.flush
;


: repl.show_history
    @ repl.history []@@
    dup repl.redraw_line

    term.flush
;


: repl.cursor_back
    variable! current_pos
    variable! start_pos

    start_pos --!
    current_pos --!

    current_pos @ start_pos @ <=
    if
        start_pos @ current_pos !
    else
        1 term.cursor_left!
        term.flush
    then
    current_pos @
;

: repl.cursor_forward
    variable! current_pos
    variable! max_pos

    current_pos ++!

    current_pos @ max_pos @ <=
    if
        1 term.cursor_right!
        term.flush
    then
;


: repl.cursor_delete!
    variable! current_line_index
    variable! index

    "" current_line_index @ @ <>
    if
        index @ current_line_index @ string.length@ >=
        if
            current_line_index @ string.length@ -- index !
        then

        index @ 0>=
        if
            1 index @ current_line_index @ string.remove!
        then
    then
;


: repl.cursor_insert!
    variable! line_index
    variable! index
    variable! text

    index @ line_index @ string.length@ >=
    if
        line_index @ @ text @ + line_index @ !
    else
        text @ index @ line_index @ string.insert!
    then
;


: repl.readline
    repl.history_index @ variable! index
    "" variable! line
    variable start_col

    "prompt" execute

    true term.raw_mode
    term.flush

    ( Get the cursor position.  This way we know how long the prompt string was. )
    term.cursor_position@ start_col !
    drop

    begin
        term.key

        ( Ctrl+C was pressed, exit gracefully. )
        dup "\03" =
        if
            exit_failure quit
        then

        dup term.esc =
        if
            drop
            term.key

            "\091" =
            if
                term.key
                case
                    ( Up arrow. )
                    "\065" of
                        index repl.decrement_history_index
                        index repl.show_history
                        line !
                    endof

                    ( Down arrow. )
                    "\066" of
                        index repl.increment_history_index
                        index repl.show_history
                        line !
                    endof

                    ( Left arrow. )
                    "\068" of
                        start_col @ -- term.cursor_column@ repl.cursor_back
                        drop
                    endof

                    ( Right arrow. )
                    "\067" of
                    line string.length@ start_col @ + term.cursor_column@ repl.cursor_forward
                    endof
                endcase

                continue
            then
        else
            ( Delete key. )
            dup "\0127" =
            if
                drop

                term.cursor_column@ start_col @ >
                if
                    ( Get the cursor position and firgure out where in the current string we are. )
                    ( Then delete one character from the string. )
                    term.cursor_column@ start_col @ - -- line repl.cursor_delete!

                    term.cursor_save
                    line @ repl.redraw_line
                    term.cursor_restore

                    1 term.cursor_left!
                    term.flush
                then

                continue
            then
        then

        dup term.is_printable?
        if
            dup "\n" <>
            if
                dup term.!

                dup term.cursor_column@ start_col @ - -- line repl.cursor_insert!

                term.cursor_save
                line @ repl.redraw_line
                term.cursor_restore
            then

            term.flush
        then

        "\013" =
    until

    false term.raw_mode

    cr

    line @
;


: repl.exit_handler
    false term.raw_mode

    "ok" .cr
    repl.save_history
;


( Define a user prompt for the REPL. )
: prompt term.csi "2:34m>" + term.csi + "0:0m>" + . ;


: repl
    "Strange Forth REPL." .cr
    cr
    "Enter quit, q, or exit to quit the REPL." .cr
    cr

    repl.load_history
    at_exit repl.exit_handler

    begin
        true
    while
        try
            cr

            repl.readline

            dup repl.insert_history
            code.execute_source

            "ok" .cr
        catch
            false term.raw_mode
            .cr
        endcatch
    repeat
;
