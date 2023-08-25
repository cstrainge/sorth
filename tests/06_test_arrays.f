
5 [].new variable! my_array

"Original:      " . my_array ?

0  my_array [ 0 ]!!
10 my_array [ 1 ]!!
20 my_array [ 2 ]!!
30 my_array [ 3 ]!!
40 my_array [ 4 ]!!

"Updated:       " . my_array @ .cr


"Read [1]:      " . my_array [ 1 ]@@ .cr
"Read [3]:      " . my_array [ 3 ]@@ .cr


10 my_array [].resize!
"Resize bigger: " . my_array ?

2 my_array [].resize!
"Resize smaller:" . my_array ?
