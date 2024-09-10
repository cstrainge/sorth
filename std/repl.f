
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
    @ variable! history
      variable! command

    command @  ""  <>
    if
        history repl.history.is_empty??
        if
            0 history repl.history.head!!
            0 history repl.history.tail!!
            1 history repl.history.count!!
        else
            history repl.history.is_full??
            if
                history repl.history.tail++!!
            else
                history repl.history.count@@  ++  history repl.history.count!!
            then

            history repl.history.head++!!
        then

        command @  history repl.history.buffer@@  [ history repl.history.head@@ ]!
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


( The state of the repl's built in editor. )
# repl.state  hidden
    is_multi_line? -> false , ( What mode are we in? )

    width -> 0 ,              ( How wide is the editor currently. )
    height -> 0 ,             ( If in multi-line, what is the editor's visible height? )

    cursor.x -> 0 ,
    cursor.y -> 0 ,

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
                endcase
            endof

        term.ctrl+c of
                repl.command.quit command !
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


: repl.multi_line_edit  hidden  ( history_var state_var -- [source_text] bool )
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


( Implementation of the single line edit mode. )
: repl.single_line_edit  hidden  ( history_var state_var -- [source_text] edit_finished )
    @ variable! state
    @ variable! history

    false variable! is_done_editing?

    variable next_key     ( Key associated with the command, if any. )

    repl.get_next_command
    case
        repl.command.up of
                state repl.state.history_index@@  --  dup  state repl.state.history_index!!
                history repl.history.relative@@

                dup state repl.state.lines@@ [ 0 ]!
                state repl.single_line.repaint

                string.size@ dup state repl.state.cursor.x!!
                term.cursor_right!
            endof

        repl.command.down of
                state repl.state.history_index@@  ++  dup  state repl.state.history_index!!

                dup  0<=
                if
                    history repl.history.relative@@  dup  state repl.state.lines@@ [ 0 ]!
                    state repl.single_line.repaint

                    string.size@ dup state repl.state.cursor.x!!
                    term.cursor_right!
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
                true is_done_editing?
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

    term.size@  state repl.state.height!!
                state repl.state.width!!

    begin
        done @  '
    while
        state repl.state.is_multi_line?@@
        if
            history state repl.multi_line_edit
        else
            history state repl.single_line_edit
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
    "*
       Strange Forth REPL.
       Version: {}

       Enter quit, q, or exit to quit the REPL.
       Enter .w to show defined words.
       Enter show_word <word_name> to list detailed information about a word.

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
