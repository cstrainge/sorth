
#pragma once



namespace sorth
{


    SORTH_API void register_builtin_words(InterpreterPtr& interpreter);


    SORTH_API void register_command_line_args(InterpreterPtr& interpreter,
                                              int argc,
                                              int args,
                                              char** argv);


}
