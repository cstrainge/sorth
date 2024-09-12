
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
            history_data { "version" }@@ "Unknown version, {}, of the history file." string.format .cr
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

    cursor.x -> 0 ,
    cursor.y -> 0 ,

    visible_lines -> 1 ,
    first_line -> 0 ,

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


: repl.multi_line.repaint_line hidden
    @ variable! state
      variable! line
;


: repl.multi_line.repaint_current_line  hidden
    @ variable! state

    state repl.state.cursor.x@@  state repl.multi_line.repaint_line
;


: repl.multi_line.close_out  hidden
;


: repl.multi_line.compute_line  hidden
    @ variable! state

    ( Compute the cursor line to the actual visible line. )
    state repl.state.cursor.y@@
;


: repl.multi_line.consolidate_string  hidden
    @ variable! state
;


: repl.multi_line.adjust_cursor.x  hidden  ( move_x state_var -- )
    @ variable! state
      variable! new_x
;


: repl.multi_line.adjust_cursor.y  hidden  ( move_y state_var -- )
    @ variable! state
      variable! new_y
;


: repl.multi_line.home  hidden
    @ variable! state
;


: repl.multi_line.end  hidden
    @ variable! state
;


: repl.multi_line.delete  hidden
    @ variable! state
;


: repl.multi-line.insert  hidden
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


: repl.multi_line.newline  hidden
    @ variable! state
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
                -1 state repl.multi_line.adjust_cursor.y
            endof

        repl.command.down of
                1 state repl.multi_line.adjust_cursor.y
            endof

        repl.command.left of
                -1 state repl.multi_line.adjust_cursor.x
            endof

        repl.command.right of
                1 state repl.multi_line.adjust_cursor.x
            endof

        repl.command.backspace of
                -1 state repl.multi_line.adjust_cursor.x

                state repl.multi-line.delete
                state repl.multi_line.repaint_current_line
            endof

        repl.command.delete of
                state repl.multi_line.delete
                state repl.multi_line.repaint_current_line
            endof

        repl.command.home of
                state repl.multi_line.home
            endof

        repl.command.end of
                state repl.multi_line.end
            endof

        repl.command.ret of
                state repl.multi_line.newline
            endof

        repl.command.quit of
                state repl.multi_line.close_out

                "exit_failure quit"
                true is_done_editing? !
            endof

        repl.command.mode_switch of
                state repl.multi_line.close_out

                state repl.multi_line.consolidate_string
                true is_done_editing? !
            endof

        repl.command.key_press of
                next_key !

                next_key @  term.is_printable?
                if
                    next_key @ state repl.multi-line.insert
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
.s
    new @
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
                true state repl.state.is_multi_line?!!
                state repl.multi_line.repaint_current_line
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


( When the repl exits make absolutely sure that the terminal is left as we found it. )
: repl.on_exit  hidden  ( -- )
    false term.raw_mode
    repl.history.state repl.history.save

    "ok" .cr
;


: repl.prompt description: "Print the user prompt.  Replace this word to customize the prompt."
              signature: " -- "
    240 term.fgc ">" + term.crst + "> " + .
;


: repl  description: "Sorth's Read Evaluate and print loop.  This word does not return."
        signature: " -- "

    ( Make sure we clean up after ourselves when exiting. )
    at_exit repl.on_exit

    ( Print the welcome banner. )
    sorth.version
    user.os "macOS" = if "⌥+return" else "alt+enter" then
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
        true
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
;
