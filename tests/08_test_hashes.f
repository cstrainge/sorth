
"world" constant value1
"to the limit!" constant value2

"hello" constant key1
"fhqwhgads" constant key2

{}.new variable! table


value1 table { key1 }!!
value2 table { key2 }!!

table ? cr

table { key1 }@@ .cr
table { key2 }@@ .cr


table { key1 }@@ value1 <>
if
    "Hash table mismatch!" .cr
    exit_failure quit
then

table { key2 }@@ value2 <>
if
    "Hash table mismatch!" .cr
    exit_failure quit
then
