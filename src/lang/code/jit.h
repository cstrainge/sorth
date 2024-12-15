
#pragma once


#ifndef SORTH_JIT_DISABLED


namespace sorth::internal
{


    // JIT compile a single immediate word into it's own module.
    WordFunction jit_immediate_word(InterpreterPtr& Interpreter,
                                    const Construction& construction);


    // JIt compile the given byte-code block into the script's top level function handler.  While
    // doing that we'll also JIT compile all of the script's non-immediate words that have been
    // cached during the byte-code compilation phase.
    //
    // We return the handler for the top level script function, all words are registered in the
    // interpreter's dictionary automaticaly.
    WordFunction jit_module(InterpreterPtr& Interpreter,
                            const std::string& name,
                            const ByteCode& code,
                            const std::map<std::string, Construction>& word_jit_cache);


}


#endif
