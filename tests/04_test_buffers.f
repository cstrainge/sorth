
( Create a byte buffer of 50 bytes. )
variable bytes
70 buffer.new bytes !


( Write a few values into the buffer. )
-1 bytes buffer.i32!!
1024.28 bytes buffer.f32!!
2048.56 bytes buffer.f64!!
"Hello world." 16 bytes buffer.string!!
"Something wicked this way comes." 32 bytes buffer.string!!
"foo" 5 bytes buffer.string!!


( Show the buffer as it is now. )
bytes ?


( Move the buffer index back to the beginning. )
0 bytes buffer.position!!


( Read the values back from the buffer. )
cr

"i32:" . bytes buffer.i32@@ .cr
"f32:" . bytes buffer.f32@@ .cr
"f64:" . bytes buffer.f64@@ .cr
"Str:" . 16 bytes buffer.string@@ .cr
"Str:" . 32 bytes buffer.string@@ .cr
"Str:" . 5 bytes buffer.string@@ .cr


( Move the buffer back to position 0 and read the first value again this time as an unsigned int. )
0 bytes buffer.position!!

"u32:" . bytes buffer.u32@@ .cr

cr


( Recreate the buffer, but make it only 8 bytes this time.  Make sure we can fill the whole thing. )
8 buffer.new bytes !

-1         bytes buffer.i32!!
0x10101010 bytes buffer.i32!!

bytes ?
