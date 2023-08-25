
( Define our structures... )
# foo a b c ;
# bar x y z ;


( Create a variable to hold an instance of foo. )
variable fp


( The raw value we get from variable creation. )
"New variable:         " . fp ?


( Extend the automaticly generated word foo.new to always create an instance of bar inn field c. )
: foo.new
    foo.new   ( Call the original version of the word... )
    dup       ( Remember, data objects are references, so we are not duplicating the struct, )
              ( just the reference to it. )

    ( Create the instance of bar, and make sure that the foo instance is at the top of the stack )
    ( so that the word #! can find it. )
    bar.new swap foo.c swap #!
;


( Create an instance of the struct foo and store it in our variable. )
foo.new fp !
"Unitialized struct:   " . fp ?


( Assign some values to the first two fields. )
1024           foo.a fp #!!
"Hello world!" foo.b fp #!!


( Now fill out the rest of foo.c's bar fields. )
150 bar.x foo.c fp #@@ #!
250 bar.y foo.c fp #@@ #!
350 bar.z foo.c fp #@@ #!

( Finally print the whole thing. )
"Initialized struct(s):" . fp ?
