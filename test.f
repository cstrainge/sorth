
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

variable fp
foo.new fp !

1024 foo.a fp #!!
foo.a fp #@@ .cr

fp @ .cr


cr

false hello
