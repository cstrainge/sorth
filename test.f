
( For testing ifs... )
: hello ( boolean -- ) if "Hello world!" else "Goodbye all." then .cr ;


( Testing loops... )
: loop_until ( -- ) 10 begin "Looping until." .cr -- dup 0<= until drop ;
: loop_while ( -- ) 10 begin -- dup 0>= while "Looping while." .cr repeat drop ;


true hello

cr

loop_until

cr

loop_while

cr


( Now lets test data object creation and access. )
# foo a b c ;
# bar x y z ;

variable fp
foo.new fp !

"---" . fp ?

1024 foo.a fp #!!
"Hello world!" foo.b fp #!!

bar.new foo.c fp #!!
150 bar.x foo.c fp #@@ #!
250 bar.y foo.c fp #@@ #!
350 bar.z foo.c fp #@@ #!

"---" . fp ?


cr

false hello
