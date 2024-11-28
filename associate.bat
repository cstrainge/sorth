%ECHO OFF

REM Associate Strange Forth Scripts with the Strange Forth Interpreter.
REM Pass the path to the interpreter to register as the command line argument.

REM This script needs to be run as administrator to register the file type.


echo "Registering %1 as the Strange Forth Interpreter..."
echo "THis script needs to be run as an administrator to register the file type."


REM Register .f files as an executable extension.  That way you don't need to specify the extension
REM when running a script.
setx PATHEXT "%PATHEXT%;.F"


REM Associate .F files as Strange Forth Scripts.
assoc .F=StrangeForthScript


REM Register the Strange Forth Interpreter as the application to run Strange Forth Scripts.
ftype StrangeForthScript=%1 "%%1" "%%*"
