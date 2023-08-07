
( Define our structures... )
# foo a b c ;
# bar x y z ;

( Create a variable to hold an instance of foo. )
variable fp
foo.new fp !

( Print out what we have so far. )
fp ?

( Assign some values to the first two fields. )
1024 foo.a fp #!!
"Hello world!" foo.b fp #!!

( Create an instance of bar and write it into foo.c )
bar.new foo.c fp #!!

( Now fill out the rest of foo.c's bar fields. )
150 bar.x foo.c fp #@@ #!
250 bar.y foo.c fp #@@ #!
350 bar.z foo.c fp #@@ #!

( Finally print the whole thing. )
fp ?
