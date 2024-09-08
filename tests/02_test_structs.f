
( Define our structures... )
# bar x y z ;

# foo
    a
    b
    c -> bar.new
;


( Create a variable to hold an instance of foo. )
variable fp


( The raw value we get from variable creation. )
"New variable:            " . fp .cr


( Create an instance of the struct foo and store it in our variable. )
foo.new fp !
"Uninitialized struct:    " . fp .cr


( Assign some values to the first two fields. )
1024           foo.a fp #!!
"Hello world!" foo.b fp #!!


( Now fill out the rest of foo.c's bar fields. )
150 bar.x foo.c fp #@@ #!
250 bar.y foo.c fp #@@ #!
350 bar.z foo.c fp #@@ #!

( Finally print the whole thing. )
"Initialized struct(s):   " . fp .cr
