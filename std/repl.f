
( Implementation of Sorth's repl.  The repl supports features like persistent command history, )
( single and multi-line editing. )

user.home user.path_sep + ".sorth_init" + constant repl.config_path


( Count of the maximum number of items that can be in the history at any one time. )
100 constant repl.history.default_max_size
user.home user.path_sep + ".sorth_history.json" + constant repl.history.path


( Keep track of the repl's history.  We're using a circular buffer capped at max_size. )
# repl.history hidden
    buffer -> repl.history.default_max_size [].new ,  ( Circular buffer of strings to hold the )
                                                      ( command history. )

    count ->  0 ,                           ( How many commands have we actually stored? )

    max -> repl.history.default_max_size ,  ( How many items can we store in total? )

    head  -> -1 ,                           ( The head of our circular buffer. )
    tail  -> -1                             ( The end of our circular buffer. )
;


( Clear out the history buffer. )
: repl.history.clear!!  hidden  ( history_var -- )
    @ variable! history

    repl.history.default_max_size [].new  history repl.history.buffer!!
    0                                     history repl.history.count!!
    repl.history.default_max_size         history repl.history.max!!
    -1                                    history repl.history.head!!
    -1                                    history repl.history.tail!!
;


( Is the history at capacity?  That is, will new items overwrite old? )
: repl.history.is_full??  hidden  ( history_var -- bool )
    @ dup repl.history.count@  swap repl.history.max@  =
;


( Is the history empty? )
: repl.history.is_empty??  hidden  ( history_var -- bool )
    @ repl.history.count@  0  =
;


( Increment a history index, wrapping it as required. )
: repl.history.inc_index  hidden  ( original_index history_var -- updated_index )
    @ variable! history
    variable! index

    index ++!

    index @  history repl.history.max@@  >=
    if
        0 index !
    then

    index @
;


( Increment the tail of a history variable. )
: repl.history.tail++!!  hidden  ( history_var -- )
    @ variable! history

    history repl.history.tail@@ history repl.history.inc_index  history repl.history.tail!!
;


( Increment the head of a history variable. )
: repl.history.head++!!  hidden  ( history_var -- )
    @ variable! history

    history repl.history.head@@ history repl.history.inc_index  history repl.history.head!!
;


( Append a new item to the head of the history list. )
: repl.history.append!!  hidden  ( new_command history_var -- )
    @ variable! history            ( The history buffer. )
      variable! command            ( The new command to add to the history. )
    false variable! is_duplicate?  ( Is this command the same as the previous one?  )

    ( If the history isn't empty check to see if the new command is a duplicate of the last )
    ( entered command. )
    history repl.history.is_empty??  '
    if
        command @  history repl.history.buffer@@ [ history repl.history.head@@ ]@  =
        is_duplicate? !
    then

    ( If this command isn't empty and a duplicate of the top command, enter it into the history. )
    is_duplicate? @ '
    command @  ""  <>
    &&
    if
        ( If this is the first command to be entered, initialize the head/tail/count. )
        history repl.history.is_empty??
        if
            0 history repl.history.head!!
            0 history repl.history.tail!!
            1 history repl.history.count!!
        else
            history repl.history.is_full??
            if
                ( The history is full, so advance the tail as the last one is about to be )
                ( overwritten. )
                history repl.history.tail++!!
            else
                ( The history isn't full yet so increment the count to account for the command )
                ( that we're about to add. )
                history repl.history.count@@  ++  history repl.history.count!!
            then

            ( Increment the head index wrapping as needed. )
            history repl.history.head++!!
        then

        ( Finally, append the new command to the buffer. )
        command @ history repl.history.buffer@@ [ history repl.history.head@@ ]!
    then
;


( Access a history command relative to the history's head.  So, 0 refers to the most recent item, )
( while -1 is the command just before that, etc.  If the index exceeds the history's capacity then )
( the last item actually stored is returned instead. )
: repl.history.relative@@  hidden  ( relative_index history_var -- command )
    @ variable! history

    variable! relative_index
    variable actual_index

    history repl.history.max@@ variable! max_size

    ( First, make sure we aren't out of range.  If we are, then just cap it at the tail. )
    0 relative_index @ -  max_size @  >=
    if
        history repl.history.tail@@  actual_index !
    else
        ( Otherwise, compute the wrapped index. )
        history repl.history.head@@  relative_index @  +  max_size @  %  actual_index !

        actual_index @  0<
        if
            max_size @  actual_index @  +  actual_index !
        then
    then

    history repl.history.buffer@@ [ actual_index @ ]@
;


( Load the history from disk from repl.history.path. )
: repl.history.load  hidden  ( history_var -- )
    @ variable! history

    repl.history.path  file.exists?
    if
        repl.history.path file.r/o file.open variable! fd
        fd @ file.size@ fd @ file.string@ variable! json_text

        json_text @ {}.from_json variable! history_data

        history_data { "version" }@@  1  <>
        if
            history_data { "version" }@@
            "Unknown version, {}, of the history file." string.format .cr
        else
            history_data { "max_items" }@@ variable! max_items
            history_data { "items" }@@ variable! items
            items [].size@@ variable! count
            0 variable! index

            history repl.history.clear!!

            history repl.history.max@@  max_items @  <>
            if
                max_items @ history repl.history.max!!
                max_items @ history repl.history.buffer@@ [].size!
            then

            begin
                index @  count @  <
            while
                items [ count @ -- index @ - ]@@  history repl.history.append!!
                index ++!
            repeat
        then

        fd @ file.close
    then
;


( Save the history back to the disk @ repl.history.path. )
: repl.history.save  hidden  ( history_var -- )
    @ variable! history

    history repl.history.count@@ variable! count
    count @ [].new variable! command_items
    0 variable! index

    begin
        index @  count @  <
    while
        0 index @ -  history repl.history.relative@@  command_items [ index @ ]!!
        index ++!
    repeat

    {
        "version" -> 1 ,
        "max_items" -> history repl.history.max@@ ,
        "items" -> command_items @
    }
    {}.to_json variable! save_text
    repl.history.path file.w/o file.create variable! fd

    save_text @ fd @ file.line!
    fd @ file.close
;


10 constant repl.multi_line.max_visible  ( The maximum number of lines the multi-line editor will )
                                         ( grow to.  The virtual text can be larger than this. )


( The state of the repl's built in editor. )
# repl.state  hidden
    is_multi_line? -> false , ( What mode are we in? )

    width -> 0 ,              ( How wide is the editor currently. )
    height -> 0 ,             ( If in multi-line, what is the editor's visible height? )

    cursor.x -> 0 ,           ( The cursor's current x position. )
    cursor.y -> 0 ,           ( The cursor's current y position. )

    x -> 0 ,                  ( Editor's upper left x corner position. )
    y -> 0 ,                  ( Editor's upper left y corner position. )

    history_index -> 1 ,      ( Index of the history item being viewed.  1 == the command being )
                              ( edited. )

    lines -> [ "" ]           ( The text that the editor is editing. )
;


: repl.state.cursor.x++!!  hidden
    dup repl.state.cursor.x@@ ++  swap repl.state.cursor.x!!
;


: repl.state.cursor.x--!!  hidden
    dup repl.state.cursor.x@@ --  swap repl.state.cursor.x!!
;


: repl.state.cursor.y++!!  hidden
    dup repl.state.cursor.y@@ ++  swap repl.state.cursor.y!!
;


: repl.state.cursor.y--!!  hidden
    dup repl.state.cursor.y@@ --  swap repl.state.cursor.y!!
;


( Set of constants for converting key presses to unified command codes. )
 0 constant repl.command.up           ( Move the cursor up a line. )
 1 constant repl.command.down         ( Move the cursor down a line. )
 2 constant repl.command.left         ( Move the cursor left one character. )
 3 constant repl.command.right        ( Move the cursor right one character. )

 4 constant repl.command.backspace    ( Delete the previous character from the cursor. )
 5 constant repl.command.delete       ( Delete the character at the cursor. )

 6 constant repl.command.home         ( Go to the beginning of the line. )
 7 constant repl.command.end          ( Go to the end of the line. )

 8 constant repl.command.ret          ( The user hit the enter key. )
 9 constant repl.command.quit         ( The user hit ctrl+c. )
10 constant repl.command.mode_switch  ( The user wants to switch edit modes. )

11 constant repl.command.key_press     ( User pressed a text key. )


( Read the terminal input and convert it to a text editor command.  If the command is a standard )
( key press then the key is also pushed as well. )
: repl.get_next_command  hidden  ( -- [key] command_id )
    term.key variable! key_pressed
    variable command

    key_pressed @
    case
        term.esc of
                term.key
                case
                    "[" of
                        term.key
                        case
                            term.up_arrow of
                                    repl.command.up command !
                                endof

                            term.down_arrow of
                                    repl.command.down command !
                                endof

                            term.right_arrow of
                                    repl.command.right command !
                                endof

                            term.left_arrow of
                                    repl.command.left command !
                                endof

                            "H" of
                                    repl.command.home command !
                                endof

                            "F" of
                                    repl.command.end command !
                                endof

                            "3" of
                                    term.key "~" =
                                    if
                                        repl.command.delete command !
                                    then
                                endof
                        endcase
                    endof

                    term.return of
                        repl.command.mode_switch command !
                    endof
                endcase
            endof

        term.ctrl+c of
                repl.command.quit command !
            endof

        term.cmd+left of
            repl.command.home command !
            endof

        term.cmd+right of
             repl.command.end command !
            endof

        term.backspace of
                repl.command.backspace command !
            endof

        term.return of
                repl.command.ret command !
            endof

        repl.command.key_press command !
        key_pressed @
    endcase

    command @
;


( Move the cursor to the beginning of the edit buffer. )
: repl.multi_line.adjust_cursor.move_to_beginning  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.x@@ variable! x
    state repl.state.cursor.y@@ variable! y

    x @  0>
    if
        x @ term.cursor_left!
    then

    y @  0>
    if
        y @ term.cursor_up!
    then

    0 state repl.state.cursor.x!!
    0 state repl.state.cursor.y!!
;


( Move the cursor to an arbitrary position within the edit buffer. )
: repl.multi_line.adjust_cursor.move_to  hidden  ( x y state_var -- )
    @ variable! state

      variable! new_y
      variable! new_x

    state repl.state.cursor.x@@ variable! x
    state repl.state.cursor.y@@ variable! y

    new_x @  x @  -  variable! x_diff
    new_y @  y @  -  variable! y_diff

    x_diff @  0<
    if
        0 x_diff @ - term.cursor_left!
    else
        x_diff @  0>
        if
            x_diff @ term.cursor_right!
        then
    then

    y_diff @  0<
    if
        0 y_diff @ - term.cursor_up!
    else
        y_diff @  0>
        if
            y_diff @ term.cursor_down!
        then
    then

    new_x @  state repl.state.cursor.x!!
    new_y @  state repl.state.cursor.y!!
;


( Move the cursor to the end of the edit buffer. )
: repl.multi_line.adjust_cursor.move_to_end  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.y@@ variable! y
    state repl.state.lines@@ [].size@ variable! count

    count @  y @  - variable! diff

    diff @  0>
    if
        diff @  term.cursor_down!
    then

    state repl.state.lines@@ [ count @ -- ]@ string.size@  dup  term.cursor_right!

               state repl.state.cursor.x!!
    count @ -- state repl.state.cursor.y!!
;


( Adjust the cursor right one position.  Making sure to wrap to the next line if we try to move )
( beyond the end of the line. )
: repl.multi_line.adjust_cursor.right  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.x@@ variable! x
    state repl.state.cursor.y@@ variable! y

    x ++!

    x @  state repl.state.lines@@ [ y @ ]@ string.size@  >
    if
        y @  state repl.state.lines@@ [].size@ --  <
        if
            y ++!
            0  x !
        else
            state repl.state.lines@@ [ y @ ]@ string.size@  x !
        then
    then

    x @  y @  state repl.multi_line.adjust_cursor.move_to
;


( Adjust the cursor left one position.  If this would move past the beginning of the line wrap to )
( the end of the previous line. )
: repl.multi_line.adjust_cursor.left  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.x@@ variable! x
    state repl.state.cursor.y@@ variable! y

    x --!

    x @  0<
    if
        y @  0  >
        if
            y --!
            state repl.state.lines@@ [ y @ ]@ string.size@  x !
        else
            0  x !
        then
    then

    x @  y @  state repl.multi_line.adjust_cursor.move_to
;


( Move the cursor up one line. )
: repl.multi_line.adjust_cursor.up  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.x@@ variable! x
    state repl.state.cursor.y@@ variable! y
    0 variable! line_size

    y --!

    y @  0>=
    if
        state repl.state.lines@@ [ y @ ]@ string.size@  line_size !

        x @  line_size @  >
        if
            line_size @  x !
        then

        x @  y @  state repl.multi_line.adjust_cursor.move_to
    then
;


( Bump the cursor down one line. )
: repl.multi_line.adjust_cursor.down  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.x@@ variable! x
    state repl.state.cursor.y@@ variable! y
    0 variable! line_size

    y ++!

    y @  state repl.state.lines@@ [].size@  <
    if
        state repl.state.lines@@ [ y @ ]@ string.size@  line_size !

        x @  line_size @  >
        if
            line_size @  x !
        then

        x @  y @  state repl.multi_line.adjust_cursor.move_to
    then
;


( Move the cursor to the beginning of the current line. )
: repl.multi_line.adjust_cursor.start_of_line  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.y@@ variable! y

    0  y @  state repl.multi_line.adjust_cursor.move_to
;


( Move the cursor to the end of the current line. )
: repl.multi_line.adjust_cursor.end_of_line  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.y@@ variable! y
    state repl.state.lines@@ [ y @ ]@ string.size@ variable! line_size

    line_size @  y @  state repl.multi_line.adjust_cursor.move_to
;


( Repaint the line at the given index.  If the cursor is currently on that line, make sure to )
( maintain it's position. )
: repl.multi_line.repaint_line  hidden  ( index state_var -- )
    @ variable! state
      variable! line

    term.clear_line
    "\r" term.!

    line @  state repl.state.lines@@ [].size@  <
    if
        line @  0  =
        if
            "repl.prompt" execute
        else
            state repl.state.x@@ --  term.cursor_right!
        then

        line @ ++ "{3} | " string.format term.!

        term.cursor_save

        state repl.state.lines@@ [ line @ ]@  term.!

        term.cursor_restore

        state repl.state.cursor.y@@  line @  =
        state repl.state.cursor.x@@  0>
        &&
        if
            state repl.state.cursor.x@@  term.cursor_right!
        then
    then
;


( Repaint the line the cursor is positioned on. )
: repl.multi_line.repaint_current_line  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.y@@  state repl.multi_line.repaint_line
;


( Repaint all lines in the current edit buffer.  As well preserve the current current cursor )
( position. )
: repl.multi_line.repaint_all_lines  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.x@@ variable! x
    state repl.state.cursor.y@@ variable! y

    state repl.state.lines@@ [].size@ variable! count
    0 variable! index

    state repl.multi_line.adjust_cursor.move_to_beginning

    begin
        index @  count @  <
    while
        state repl.multi_line.repaint_current_line
        state repl.multi_line.adjust_cursor.down

        index ++!
    repeat

    x @  y @  state repl.multi_line.adjust_cursor.move_to
;


( Insert a new character into the buffer at the current cursor location. )
: repl.multi_line.insert  hidden  ( new_char state_var -- )
    @ variable! state
      variable! char

    state repl.state.lines@@ [ state repl.state.cursor.y@@ ]@ variable! line
    line @ string.size@ variable! size
    state repl.state.cursor.x@@ variable! position

    position @  size @  >=
    if
        line @ char @ +  line !
    else
        char @ position @ line @ string.[]!  line !
    then

    line @ state repl.state.lines@@ [ state repl.state.cursor.y@@ ]!
;


( Delete a character from the buffer at the current cursor position. )
: repl.multi_line.delete  hidden  ( state_var -- )
    @ variable! state

    state repl.state.cursor.x@@ variable! x
    state repl.state.cursor.y@@ variable! y

    state repl.state.lines@@ variable! lines

    ( If we're at the end of the line, we're removing the previous one, if it it exists.  In that )
    ( case we append the previous line onto this one. )
    x @  lines [ y @ ]@@ string.size@  =
    if
        y @ ++  lines [].size@@  <
        if
            lines [ y @ ]@@  lines [ y @ ++ ]@@  +  lines [ y @ ]!!
            y @ ++  lines @  [].delete

            ( Repaint the buffer. )
            state repl.multi_line.repaint_all_lines
        then
    else
        lines [ y @ ]@@  string.size@  0>
        if
            ( Looks like we're just deleting from within the current line. )
            1  x @  lines [ y @ ]@@  string.remove  lines [ y @ ]!!
            state repl.multi_line.repaint_current_line
        then
    then
;


( Insert a new line into the edit buffer at the current cursor position. )
: repl.multi_line.newline  hidden  ( state_var -- )
    @ variable! state

    state repl.state.lines@@ variable! lines  ( Grab the current edit buffer. )

    ( Cache the cursor position. )
    state repl.state.cursor.x@@ variable! x
    state repl.state.cursor.y@@ variable! y

    ( Create the new line. )
    ""  y @ ++  lines @  [].insert

    ( If the cursor isn't at the end of the line, move all remaining text into the new line. )
    x @  lines [ y @ ]@@ string.size@  <
    if
        x @  string.npos  lines [ y @ ]@@  string.substring  lines [ y @ ++ ]!!
        string.npos  x @  lines [ y @ ]@@  string.remove  lines [ y @ ]!!
    then

    ( Move to the end of the buffer and make sure we make room for the new line. )
    state repl.multi_line.adjust_cursor.move_to_end
    "\r\n" term.!

    0  y @ ++  state repl.multi_line.adjust_cursor.move_to

    ( Refresh the editor display. )
    state repl.multi_line.repaint_all_lines
;


( Take the text from the editor buffer and convert it to a consolidated string for execution in )
( the repl. )
: repl.multi_line.consolidate_string  hidden  ( state_var -- edited_text )
    @ variable! state

    state repl.state.lines@@ variable! lines

    0 variable! index
    "" variable! output

    begin
        index @  lines @ [].size@  <
    while
        output @  lines [ index @ ]@@  +
        index @  lines [].size@@ --  <  if "\n" + then

        output !

        index ++!
    repeat

    output @
;


( Implementation of the multi-lime edit mode. )
: repl.multi_line.edit  hidden  ( history_var state_var -- [source_text] bool )
    @ variable! state
    @ variable! history

    false variable! is_done_editing?  ( Has the user finished editing the text? )
    variable next_key                 ( Key associated with the command, if any. )

    ( Get the command from the user and figure out what to do. )
    repl.get_next_command
    case
        repl.command.up of
                state repl.multi_line.adjust_cursor.up
            endof

        repl.command.down of
                state repl.multi_line.adjust_cursor.down
            endof

        repl.command.left of
                state repl.multi_line.adjust_cursor.left
            endof

        repl.command.right of
                state repl.multi_line.adjust_cursor.right
            endof

        repl.command.backspace of
                state repl.multi_line.adjust_cursor.left
                state repl.multi_line.delete
            endof

        repl.command.delete of
                state repl.multi_line.delete
            endof

        repl.command.home of
                state repl.multi_line.adjust_cursor.start_of_line
            endof

        repl.command.end of
                state repl.multi_line.adjust_cursor.end_of_line
            endof

        repl.command.ret of
                state repl.multi_line.newline
            endof

        repl.command.quit of
                state repl.multi_line.adjust_cursor.move_to_end
                "\r\n" term.!

                "exit_failure quit"
                true is_done_editing? !
            endof

        repl.command.mode_switch of
                state repl.multi_line.consolidate_string
                dup history repl.history.append!!

                state repl.multi_line.adjust_cursor.move_to_end
                "\r\n" term.!

                true is_done_editing? !
            endof

        repl.command.key_press of
                next_key !

                next_key @  term.is_printable?
                if
                    next_key @ state repl.multi_line.insert

                    state repl.state.cursor.x++!!
                    state repl.multi_line.repaint_current_line
                then
            endof
    endcase

    is_done_editing? @
;


( Insert a character into the text at the given cursor position. )
: repl.single_line.insert  hidden  ( key state_var -- )
    @ variable! state
    variable! key

    state repl.state.lines@@ [ 0 ]@ variable! line
    line @ string.size@ variable! size
    state repl.state.cursor.x@@ variable! position

    position @  size @  >=
    if
        line @ key @ +  line !
    else
        key @ position @ line @ string.[]!  line !
    then

    line @ state repl.state.lines@@ [ 0 ]!
;


( Delete the character at the current cursor position. )
: repl.single_line.delete  hidden  ( state_var -- )
    @ variable! state
    state repl.state.lines@@ [ 0 ]@ variable! line
    state repl.state.cursor.x@@ variable! position

    1 position @ line @ string.remove  state repl.state.lines@@ [ 0 ]!
;


( Redraw the editor text. )
: repl.single_line.repaint  hidden  ( state_var -- )
    @ variable! state

    term.clear_line
    "\r" term.!

    "repl.prompt" execute

    term.cursor_save

    state repl.state.lines@@ [ 0 ]@  term.!

    term.cursor_restore

    term.flush
;


( Take a command that potentially has \n new lines and "flatten" it to fit on one editor line. )
( This is done to easily display the command in the single line mode of the editor.  If the user )
( then switches to multi-line mode, the original new lines and whitespace are retained. )
: repl.flatten_command  hidden  ( command -- flat_version )
       variable! original
    "" variable! new

    original @ string.size@ variable! size
    0 variable! index
    variable next
    false variable! is_in_string?

    ( Copy original to new while filtering characters. )
    begin
        index @  size @  <
    while
        index @ original @ string.[]@  next !

        next @
        case
            "\n" of
                    ( Don't copy, unless the newline is in a string, if it is, filter it. )
                    is_in_string? @
                    if
                        new @  "\n"  +  new !
                    else
                        ( Make sure to preserve a space if the next text is at the beginning of )
                        ( beginning of the next line. )
                        index @ ++  size @  <
                        if
                            index @ ++  original @ string.[]@  " " <>
                            index @ ++  original @ string.[]@  "\n" <>
                            &&
                            if
                               new @  " "  +  new !
                            then
                        then
                    then
                endof

            "\\" of
                    index @ ++  size @  <
                    if
                        index @ ++  original @  string.[]@  "\""  =
                        if
                            index @ ++  index !
                            new @  "\\\""  +  new !
                        else
                            new @  next @  +  new !
                        then
                    else
                        new @  next @  +  new !
                    then
                endof

            "\"" of
                    is_in_string? @  '  is_in_string? !
                    new @  next @  +  new !
                endof

            " " of
                    ( Only copy if the next character isn't also a space, and it isn't the last )
                    ( character in the command.  If it looks like we're in a string preserve the )
                    ( whitespace. )
                    index @ ++  size @  <
                    if
                        index @ ++  original @  string.[]@  " "  <>
                        is_in_string? @
                        ||
                        if
                            new @  next @  +  new !
                        then
                    then
                endof

            ( Otherwise just copy the character. )
            new @  next @  +  new !
        endcase

        index ++!
    repeat

    new @
;


( Switch editor state to multi-line.  If the editor is on a history item that includes newlines )
( unflatten the text and populate the editor with properly formatted text. )
: repl.editor.switch_to_multi_line hidden  ( history_var state_var -- )
    @ variable! state
    @ variable! history

    true state repl.state.is_multi_line?!!

    variable count
    0 variable! index

    state repl.state.cursor.x@@ term.cursor_left!
    0 state repl.state.cursor.x!!

    1 state repl.state.history_index@@ <>
    if
        "\n" state repl.state.history_index@@ history repl.history.relative@@  string.split

        state repl.state.lines!!

        state repl.state.lines@@ [].size@  0>
        if
            state repl.state.lines@@ [].size@ count !

            begin
                index @  count @  --  <
            while
                "\n" term.!
                index ++!
            repeat

            count @ term.cursor_up!
        then
    then

    state repl.multi_line.repaint_all_lines
;


( Implementation of the single line edit mode. )
: repl.single_line.edit  hidden  ( history_var state_var -- [source_text] edit_finished )
    @ variable! state
    @ variable! history

    false variable! is_done_editing?

    variable next_key     ( Key associated with the command, if any. )

    repl.get_next_command
    case
        repl.command.up of
                state repl.state.history_index@@  --  dup  state repl.state.history_index!!
                history repl.history.relative@@  repl.flatten_command

                dup state repl.state.lines@@ [ 0 ]!
                state repl.single_line.repaint

                string.size@ dup state repl.state.cursor.x!!  term.cursor_right!
            endof

        repl.command.down of
                state repl.state.history_index@@  ++  dup  state repl.state.history_index!!

                dup  0<=
                if
                    history repl.history.relative@@ repl.flatten_command

                    dup  state repl.state.lines@@ [ 0 ]!
                    state repl.single_line.repaint

                    string.size@ dup state repl.state.cursor.x!!  term.cursor_right!
                else
                    1 state repl.state.history_index!!

                    1  =
                    if
                        "" state repl.state.lines@@ [ 0 ]!
                        0 state repl.state.cursor.x!!
                        state repl.single_line.repaint
                    then
                then
            endof

        repl.command.left of
                state repl.state.cursor.x@@ -- 0>=
                if
                    state repl.state.cursor.x--!!
                    1 term.cursor_left!
                then
            endof

        repl.command.right of
                state repl.state.cursor.x@@ ++  state repl.state.lines@@ [ 0 ]@ string.size@  <=
                if
                    state repl.state.cursor.x++!!
                    1 term.cursor_right!
                then
            endof

        repl.command.backspace of
                state repl.state.cursor.x@@  0>
                if
                    state repl.state.cursor.x--!!
                    state repl.single_line.delete

                    state repl.single_line.repaint

                    state repl.state.cursor.x@@ 0>
                    if
                        state repl.state.cursor.x@@ term.cursor_right!
                    then
                then
            endof

        repl.command.delete of
                state repl.state.cursor.x@@  state repl.state.lines@@ [ 0 ]@ string.size@  <
                if
                    state repl.single_line.delete

                    state repl.single_line.repaint
                    state repl.state.cursor.x@@ term.cursor_right!
                then
            endof

        repl.command.home of
                state repl.state.cursor.x@@  0>
                if
                    state repl.state.cursor.x@@ term.cursor_left!
                    0 state repl.state.cursor.x!!
                then
            endof

        repl.command.end of
                state repl.state.cursor.x@@  state repl.state.lines@@ [ 0 ]@ string.size@  <
                if
                    state repl.state.lines@@ [ 0 ]@ string.size@  state repl.state.cursor.x@@  -
                    term.cursor_right!

                    state repl.state.lines@@ [ 0 ]@ string.size@  state repl.state.cursor.x!!
                then
            endof

        repl.command.ret of
                "\r\n" term.!
                state repl.state.lines@@ [ 0 ]@
                dup history repl.history.append!!
                true is_done_editing? !
            endof

        repl.command.quit of
                "\r\n" term.!
                "exit_failure quit"
                true is_done_editing? !
            endof

        repl.command.mode_switch of
                history state repl.editor.switch_to_multi_line
            endof

        repl.command.key_press of
                next_key !

                next_key @  term.is_printable?
                if
                    next_key @ state repl.single_line.insert

                    state repl.single_line.repaint
                    state repl.state.cursor.x++!!
                    state repl.state.cursor.x@@ term.cursor_right!
                then
            endof
    endcase

    is_done_editing? @
;


( Edit user input and return the text they created. )
: repl.readline@@  hidden  ( history_var -- user_command )
    @ variable! history

    repl.state.new variable! state
    false variable! done

    true term.raw_mode

    "repl.prompt" execute

    term.cursor_position@  state repl.state.x!!
                           state repl.state.y!!

    term.size@  drop  1 state repl.state.height!!
                state repl.state.x@@ - 1 -  state repl.state.width!!

    begin
        done @  '
    while
        state repl.state.is_multi_line?@@
        if
            history state repl.multi_line.edit
        else
            history state repl.single_line.edit
        then

        done !
    repeat

    false term.raw_mode
;


( Keep track of a history of commands entered in the repl. )
repl.history.new variable! repl.history.state


: repl.prompt description: "Print the user prompt.  Replace this word to customize the prompt."
              signature: " -- "
    240 term.fgc ">" + term.crst + "> " + .
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


: repl  description: "Sorth's Read Evaluate and print loop."
        signature: " -- "

    ( Print the welcome banner. )
    sorth.version
    user.os "macOS" = if "⌥ + return" else "alt + enter" then
    "*
       Strange Forth REPL.
       Version: {}

       Enter quit, q, or exit to quit the REPL.
       Enter .w to show defined words.
       Enter show_word <word_name> to list detailed information about a word.
       Hit {} to enter multi-line editing mode.

    *"
    string.format .cr

    ( Load the previous session's history if there is one. )
    repl.history.state repl.history.load

    ( Load and process the user config file, if it exists. )
    repl.config_path file.exists?
    if
        repl.config_path include
    then

    ( Loop forever.  If the user enters a quit command the execution of this script will end at )
    ( that point. )
    begin
        repl.is_quitting? @ '
    while
        try
            ( Read and attempt to execute the user command. )
            repl.history.state repl.readline@@
            code.execute_source

            ( If we get here, everything ran ok. )
            "ok" .cr cr
        catch
            ( Make sure to reset the terminal, if it hasn't been already. )
            false term.raw_mode

            ( An error occurred so report the error to the user. )
            cr .cr
        endcatch
    repeat

    false term.raw_mode
    repl.history.state repl.history.save
;
