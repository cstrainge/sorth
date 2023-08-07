
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
