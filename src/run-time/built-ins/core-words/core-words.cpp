
#include "sorth.h"

#include <stdlib.h>
#include <cstdlib>
#include <iomanip>

#include "array-words.h"
#include "byte-buffer-words.h"
#include "byte-code-words.h"
#include "hash-table-words.h"
#include "interpreter-words.h"
#include "math-logic-words.h"
#include "stack-words.h"
#include "string-words.h"
#include "structure-words.h"
#include "token-words.h"
#include "value-type-words.h"
#include "word-creation-words.h"
#include "word-info-struct.h"
#include "word-words.h"



namespace sorth
{


    using namespace internal;


    void register_builtin_words(InterpreterPtr& interpreter)
    {
        register_array_words(interpreter);
        register_buffer_words(interpreter);
        register_bytecode_words(interpreter);
        register_hash_table_words(interpreter);
        register_interpreter_words(interpreter);
        register_math_logic_words(interpreter);
        register_stack_words(interpreter);
        register_string_words(interpreter);
        register_structure_words(interpreter);
        register_token_words(interpreter);
        register_value_type_words(interpreter);
        register_word_creation_words(interpreter);
        register_word_words(interpreter);

        register_word_info_struct(LOCATION_HERE(), interpreter);


    }


}
