
#ifndef __SORTH_EXT_H_
#define __SORTH_EXT_H_



#ifndef __cplusplus
    #include <stdlib.h>
    #include <stdint.h>
#else
    #include <cstdlib>
    #include <cstdint>
#endif



#ifdef __cplusplus
extern "C"
{
#endif



typedef struct Value_t* ValueRef_t;

typedef struct Interpreter_t* InterpreterRef_t;

typedef struct SorthApi_t* SorthApiRef_t;

typedef int BOOL;

typedef struct HandlerResult
{
    BOOL was_successful;
    char* error_message;
}
HandlerResult_t;

typedef HandlerResult_t (*WordHandlerRef_t)(InterpreterRef_t, SorthApiRef_t);


typedef struct SorthApi_t
{
    ValueRef_t (*new_value)();
    void (*free_value)(ValueRef_t value);

    int64_t (*as_int)(InterpreterRef_t interpreter_ref, ValueRef_t value_ref);
    double (*as_float)(InterpreterRef_t interpreter_ref, ValueRef_t value_ref);
    BOOL (*as_bool)(InterpreterRef_t interpreter_ref, ValueRef_t value_ref);
    char* (*as_string)(InterpreterRef_t interpreter_ref, ValueRef_t value_ref);
    void (*free_string)(char* str);

    BOOL (*is_numeric)(ValueRef_t value_ref);
    BOOL (*is_string)(ValueRef_t value_ref);

    void (*set_int)(ValueRef_t value_ref, int64_t new_value);
    void (*set_float)(ValueRef_t value_ref, double new_value);
    void (*set_bool)(ValueRef_t value_ref, BOOL new_value);
    void (*set_string)(ValueRef_t value_ref, const char* new_value);

    void (*halt)(InterpreterRef_t interpreter);
    void (*clear_halt_flag)(InterpreterRef_t interpreter);


    void (*push)(InterpreterRef_t interpreter, ValueRef_t value);
    ValueRef_t (*pop)(InterpreterRef_t interpreter);

    void (*add_word)(InterpreterRef_t interpreter,
                     const char* name,
                     WordHandlerRef_t handler,
                     const char* file,
                     size_t line,
                     BOOL is_immediate,
                     const char* description,
                     const char* signature);
}
SorthApi_t;



typedef void (*HandlerRegistrationRef_t)(InterpreterRef_t, SorthApiRef_t);



#ifdef __cplusplus
}
#endif



#endif  // __SORTH_EXT_H_
