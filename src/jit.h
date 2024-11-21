
#pragma once


#ifndef SORTH_JIT_DISABLED


namespace sorth::internal
{


    enum class CodeGenType
    {
        word,
        script_body
    };


    WordFunction jit_bytecode(InterpreterPtr& Interpreter,
                              std::string& name,
                              CodeGenType type,
                              const ByteCode& code);


}


#endif
