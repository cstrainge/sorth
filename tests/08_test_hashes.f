
"world" constant value1
"to the limit!" constant value2

"hello" constant key1
"fhqwhgads" constant key2

{}.new variable! table


value1 key1 table {}!!
value2 key2 table {}!!

table ?

key1 table {}@@ .cr
key2 table {}@@ .cr


key1 table {}@@ value1 <>
if
    "Hash table mismatch!"
    exit_failure quit
then

key2 table {}@@ value2 <>
if
    "Hash table mismatch!"
    exit_failure quit
then
