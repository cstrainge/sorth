
: ffi.load immediate description: "Load an external library and register it with the ffi interface."
                     signature: "ffi.load library_name as name"
    ( Get the library name. )
    word

    ( Make sure the syntax is followed. )
    word dup  "as"  <>
    if
        "Expected as but found {}" string.format throw
    then

    drop

    ( With the library name and the alias we're getting now, call the native word to do the actual )
    ( library load. )
    word  ffi.load
;



: ffi.fn immediate description: "Register a function from a shared library"
                   signature: "ffi.fn library-name fn-name [as alias] input-types -> return-type"
    word variable! lib-name  ( Name of the library to reference. )
    word variable! fn-name   ( Name of the function we're binding. )
    "" variable! fn-alias    ( Optional alias to use for the function. )

    0 [].new variable! fn-params  ( List of parameter types. )

    ( Check if there's an alias for the new word. )
    word variable! next

    next @  "as"  =
    if
        word  fn-alias !
        word  next !
    then

    ( Load the parameter types until we get to the return keyword. )
    begin
        next @  "->"  <>
    while
        next @  fn-params [].push_back!!
        word next !
    repeat

fn-params @ "-[{}]-" string.format .cr

    ( Finally call the native word to finish the registration process. )
    lib-name @   op.push_constant_value
    fn-name @    op.push_constant_value
    fn-alias @   op.push_constant_value
    fn-params @  op.push_constant_value
    word         op.push_constant_value
    ` ffi.fn     op.execute
;



: as  description: "Part of the ffi loading syntax."
    "as" sentinel_word
;



: ffi.void   "ffi.void"   sentinel_word ;
: ffi.i8     "ffi.i8"     sentinel_word ;
: ffi.u8     "ffi.u8"     sentinel_word ;
: ffi.i16    "ffi.i16"    sentinel_word ;
: ffi.u16    "ffi.u16"    sentinel_word ;
: ffi.i32    "ffi.i32"    sentinel_word ;
: ffi.u32    "ffi.u32"    sentinel_word ;
: ffi.f32    "ffi.f32"    sentinel_word ;
: ffi.f64    "ffi.f64"    sentinel_word ;
: ffi.string "ffi.string" sentinel_word ;
