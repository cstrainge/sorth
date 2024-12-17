
#pragma once


namespace sorth::internal
{


    // Information about a word or top level code for a script as it's being constructed.  A
    // stack of these constructions is managed by the compile context to manage creating new
    // words and code blocks.
    //
    // When the block of code is the top level of the script, most of these fields are not used.
    // However when it's a new word being defined, all of these fields are used.
    struct Construction
    {
        bool is_immediate = false;
        bool is_hidden = false;
        bool is_context_managed = false;

        std::string name;
        std::string description;
        std::string signature;
        Location location;
        ByteCode code;
    };


    // Stack of constructions that are being managed by the compile context.
    using ConstructorStack = std::stack<Construction>;


    // Cache of words that still need to be JIT compiled.
    using JitCacheMap = std::unordered_map<std::string, Construction>;


    // Where in the bytecode list should new code be inserted?
    enum class CodeInsertionPoint
    {
        // Insert the new code at the beginning of the list.
        beginning,

        // Insert the new code at the end of the list.
        end
    };


    // The compile context is used to manage the construction of new words and code blocks in the
    // interpreter.  It's a stack of constructions that are used to manage the creation of the in
    // memory representation of the script.  If a script loads another script via an include
    // statement, a new compile context is created to manage the new script.
    class CompileContext
    {
        public:
            // If jit is enabled, this is the cache of words yet to be jit compiled.  If a word
            // is immediate, it's JIT compiled immediately.  If not, it's left as bytecode until
            // the entire script has been compiled, then all remaining words and the top level
            // of the script are JIT compiled.
            #if (SORTH_LLVM_FOUND == 1)
                std::map<std::string, Construction> word_jit_cache;
            #endif

        private:
            // Maintain a link to the interpreter that owns this compile context.
            std::weak_ptr<Interpreter> interpreter_wptr;

            // Maintain a link to the interpreter that owns this compile context.
            // The tokens that are being compiled in this script context.
            TokenList input_tokens;

            // The current token being compiled.
            size_t current_token = 0;

            // The insertion point for new code.
            CodeInsertionPoint insertion_point = CodeInsertionPoint::end;

            // The stack of constructions that are being managed by this compile context.
            ConstructorStack stack;

        public:
            CompileContext(InterpreterPtr& new_interpreter, TokenList&& tokens) noexcept;
            ~CompileContext() noexcept = default;

        public:
            // Compile tokens until one of the given words is found.  This is used to compile
            // blocks of code that are terminated by a specific word or words.  If the end of the
            // token list is reached before a match is found, an error is thrown.
            //
            // The word actually found is returned to the caller.
            std::string compile_until_words(const std::vector<std::string>& word_list);

            // Compile a single token into the current construction.
            void compile_token(const Token& token);

            // Compile the entire token list into the current construction(s).
            void compile_token_list();

        public:
            // Get the next token in the token stream, advancing the current index.  Or we throw an
            // error if we're at the end of the token list.
            const Token& get_next_token();

        public:
            // Create a new construction and push it onto the stack.
            void new_construction() noexcept;

            // Create a new construction with a name and location, and push it onto the stack.
            void new_construction(const std::string& name, const Location& location) noexcept;

            // Create a new construction with the given bytecode and push it onto the stack.
            void new_construction(ByteCode&& code) noexcept;


            // Drop the current top construction from the stack, returning it.
            Construction drop_construction();


            // Merge the top construction into the one below it.
            void merge_construction() noexcept;


            // Get the current top construction on the stack.
            Construction& construction();

        public:
            // Set the insertion point for new code.
            void set_insertion(CodeInsertionPoint point) noexcept;

            // Insert a new instruction into the current construction at the current insertion
            // point.
            void insert_instruction(const Instruction& instruction) noexcept;
    };


    // Stack of compile contexts, used by the interpreter to manage the compilation of multiple
    // scripts at one time.
    using CompileContextStack = std::stack<CompileContext>;


}
