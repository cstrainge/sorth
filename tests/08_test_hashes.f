
"world" constant value1
"to the limit!" constant value2

"hello" constant key1
"fhqwhgads" constant key2


{
    key1 -> value1 ,
    key2 -> value2
}
variable! table


table ?


cr

key1 . table @ { key1 }@ .cr
key2 . table @ { key2 }@ .cr



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

cr

table @ {}.to_json .cr
