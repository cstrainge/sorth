
#include <iostream>
#include "sorth_ext.h"



HandlerResult_t word_hello(InterpreterRef_t interpreter, SorthApiRef_t api)
{
    auto value = api->new_value();

    api->set_string(value, "Hello world!");
    api->push(interpreter, value);
    api->free_value(value);

    return HandlerResult_t { .was_successful = true };
}



extern "C" void register_module_words(InterpreterRef_t interpreter, SorthApiRef_t api)
{
    std::cout << "Module loaded." << std::endl;

    api->add_word(interpreter,
                  "sample.hello",
                  word_hello,
                  __FILE__,
                  __LINE__,
                  false,
                  "Push a message onto the stack.",
                  " -- message");
}
