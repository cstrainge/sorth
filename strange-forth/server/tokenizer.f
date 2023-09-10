
( Remember zero based lines and columns! )

# tk.location uri line column ;


: tk.location.uri!     tk.location.uri      swap @ #! ;
: tk.location.line!    tk.location.line     swap @ #! ;
: tk.location.column!  tk.location.column   swap @ #! ;

: tk.location.uri@     tk.location.uri      swap @ #@ ;
: tk.location.line@    tk.location.line     swap @ #@ ;
: tk.location.column@  tk.location.column   swap @ #@ ;


: tk.location.new  ( uri -- tk.location )
    tk.location.new variable! location

      location tk.location.uri!
    0 location tk.location.line!
    0 location tk.location.column!

    location @
;


: tk.location.inc  ( character tk.location --  )
    variable! location

    "\n" =
    if
        ( We're incrementing lines, so reset colum and increment the line. )
        0 location json.location.column!!
        location tk.location.line@ ++ location tk.location.line!
    else
        ( This isn't a new line, so we're just incrementing the column. )
        location tk.location.column@ ++ location tk.location.column!
    then
;




# tk.token location text ;




# tk.token_list items position ;


: tk.token_list.peek
;

: tk.token_list.next
;



: tk.tokenize  ( source_text -- tk.token_list )
;
