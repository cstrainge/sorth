
5 [].new variable! my_array

"Original:       " . my_array ?

10 my_array [ 0 ]!!
20 my_array [ 1 ]!!
30 my_array [ 2 ]!!
40 my_array [ 3 ]!!
50 my_array [ 4 ]!!

"Updated:        " . my_array @ .cr


"Read [1]:       " . my_array [ 1 ]@@ .cr
"Read [3]:       " . my_array [ 3 ]@@ .cr

"Read [ 1 3 2 ]: " . my_array [ 1 , 3 , 2 ]@@ . . .cr
"Write [ 1 3 2 ]:" . 1024 2048 4096 my_array [ 1 , 3 , 2 ]!! my_array ?

10 my_array [].resize!
"Resize bigger:  " . my_array ?

2 my_array [].resize!
"Resize smaller: " . my_array ?
