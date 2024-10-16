
ffi.load TestLib.dll as test-lib


ffi.# ffi.point
    ffi.i64 x -> 1 ,
    ffi.i64 y -> 1
;


ffi.fn test-lib test_function1 as tf1 ffi.string ffi.f64 -> ffi.f64
ffi.fn test-lib test_function2 as tf2 ffi.i64 ffi.i64 -> ffi.i64
ffi.fn test-lib test_function3 as tf3 ffi.point -> ffi.bool



"Hello and testing." 10.24 tf1 .cr
20 40 tf2 .cr

ffi.# ffi.point
    ffi.i64 x -> 1 ,
    ffi.i64 y -> 1
;

ffi.fn test-lib test_function3 as tf3 ffi.point -> ffi.i32

ffi.point.new variable! test_point

test_point @ dup .cr "Is in range? " . tf3 .cr


200 test_point ffi.point.y!!
test_point @ dup .cr "Is in range? " . tf3 .cr
