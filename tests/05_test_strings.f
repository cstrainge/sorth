
"\027"    constant esc  ( Terminal escape character. )
esc "[" + constant csi  ( Control sequence introducer. )


( Write the number at the top of the stack in the color of that index. )

: write_color_number  ( color_index -- )
    dup
    csi "38;5;" +
    swap +
    "m" +
    swap +
    " " +
    .
;


( Loop through the color palette of the console's 256 color mode and print the number of the color )
( in that color. )

0
begin
    dup
    write_color_number

    ++ dup

    256 =
until

drop


( Reset the color back to the terminal's default. )
csi "0;0m" + .cr


cr cr

"Hello world!" dup string.size@ --
swap over 1 rot string.remove "." + .cr
"Cliché but... " swap 0 swap string.[]! .cr

cr

"*
    This is a multi line string.
    We're including extra whitespace.

        * This line is indented!

This line is different!

    All done!
*"
.


"Should say 123: " .  1 3 "012345" string.substring  .cr

"world" 1024 2048 "Hello {}, the value is {} and that's less than {}!" string.format .cr
