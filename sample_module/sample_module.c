
#include "sorth_ext.h"



#ifdef _WIN32
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT
#endif



HandlerResult_t word_hello(InterpreterRef_t interpreter, SorthApiRef_t api)
{
    ValueRef_t value = api->new_value();

    api->set_string(value, "Hello world!");
    api->push(interpreter, value);
    api->free_value(value);

    HandlerResult_t result = { 1 };

    return result;
}



DLL_EXPORT void register_module_words(InterpreterRef_t interpreter, SorthApiRef_t api)
{
    api->add_word(interpreter,
                  "sample.hello",
                  word_hello,
                  __FILE__,
                  __LINE__,
                  0,
                  "Push a message onto the stack.",
                  " -- message");
}
