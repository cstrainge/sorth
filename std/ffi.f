
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



: ffi.void description: "Corresponds to a C void type."
    "ffi.void" sentinel_word
;

: ffi.bool description: "Corresponds to a C boolean type."
    "ffi.bool" sentinel_word
;

: ffi.i8 description: "Corresponds to a C 8 bit integer type."
    "ffi.i8" sentinel_word
;

: ffi.u8 description: "Corresponds to a C 8 bit unsigned integer type."
    "ffi.u8" sentinel_word
;

: ffi.i16 description: "Corresponds to a C 16 bit integer type."
    "ffi.i16" sentinel_word
;

: ffi.u16 description: "Corresponds to a C 16 bit unsigned integer type."
    "ffi.u16" sentinel_word
;

: ffi.i32 description: "Corresponds to a C 32 bit integer type."
    "ffi.i32" sentinel_word
;

: ffi.u32 description: "Corresponds to a C 32 bit unsigned integer type."
    "ffi.u32" sentinel_word
;

: ffi.f32 description: "Corresponds to a C 32 bit floating point type."
    "ffi.f32" sentinel_word
;

: ffi.f64 description: "Corresponds to a C 64 bit floating point type."
    "ffi.f64" sentinel_word
;

: ffi.string description: "Corresponds to a C string type."
    "ffi.string" sentinel_word
;

: ffi.void-ptr description: "Corresponds to a C pointer type."
    "ffi.void-ptr" sentinel_word
;




: ffi.# immediate description: "Create a structure compatible with the ffi interface."
                  signature: "ffi# type field -> default ... ;"
    word variable! struct_name
    false variable! is_hidden

    4 variable! alignment

    0 [].new variable! types
    0 [].new variable! field_names
    0 [].new variable! defaults

    false variable! found_initializers

    variable next_word
    0 variable! index

    code.new_block

    begin
        word next_word !

        next_word @ ";" <>
    while
        next_word @
        case
            "hidden" of
                    true is_hidden !
                    continue
                endof

            "align" of
                    word alignment !
                    continue
                endof

            "{" of
                    "(" execute
                    continue
                endof

            "->" of
                    true found_initializers !

                    ` dup op.execute
                    ";" "," 2 code.compile_until_words

                    ` swap op.execute
                    index @ -- op.push_constant_value
                    ` swap op.execute
                    ` []! op.execute

                    ";" =
                    if
                        break
                    then
                endof

            ( Get the type and name of the field. )
            index ++!

            ( Expand all our buffers. )
            index @ types [].size!!
            index @ field_names [].size!!
            index @ defaults [].size!!

            ( Use the word we have as a type name, and get the next word for the field name. )
            next_word @ types       [ index @ -- ]!!
            word        field_names [ index @ -- ]!!
        endcase
    repeat

    found_initializers @
    if
        true code.insert_at_front
        defaults @ op.push_constant_value
        false code.insert_at_front
    then

    struct_name @        op.push_constant_value
    alignment @          op.push_constant_value
    field_names @        op.push_constant_value
    types @              op.push_constant_value
    is_hidden @          op.push_constant_value
    found_initializers @ op.push_constant_value

    ` ffi.#              op.execute

    code.merge_stack_block
;




(
ffi.# point packing 1
    ffi.i32 x -> 0 ,
    ffi.i32 y -> 0
;

ffi.# Rectangle packing 8
    point-ptr top-left -> point.new ,
    point-ptr bottom-right -> point.new ,
    ffi.string label
; )


(
[include] std/ffi.f

ffi.load libm.so.6 as lib_m
ffi.fn lib_m cos as math.cos ffi.f64 -> ffi.f64

0.5 dup math.cos "cos({}) = {}" string.format .cr


ffi.load lib_my_functions.so as my_lib
ffi.fn my_lib example_function ffi.string ffi.f64 -> ffi.64
)
