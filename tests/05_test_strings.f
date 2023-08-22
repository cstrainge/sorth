
"\027"    constant esc  ( Terminal escape character. )
esc "[" + constant csi  ( Control sequence introducer. )


( Write the number at the top of the stack in the color of that index. )

: write_color_number  ( color_index -- )
    dup
    csi "38;5;" +
    swap +
    "m" +
    swap +
    .
;


( Loop through the color pallete of the console's 256 color mode and print the number of the color )
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
