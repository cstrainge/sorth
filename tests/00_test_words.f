
( For testing ifs... )
: hello ( boolean -- ) if "Hello world!" else "Goodbye all." then .cr ;


true hello
false hello


10 0 >
if
    "The 10 is greater than 0!" .cr
then

 "What about failed ifs?" .cr

10 0 <
if
    "This code problably shouldn't run!" .cr
    exit_failure quit
then

"All done." .cr

.s
