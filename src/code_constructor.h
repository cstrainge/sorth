
#pragma once


namespace sorth::internal
{


    struct Construction
    {
        bool is_immediate;
        bool is_hidden;

        std::string name;
        std::string description;
        std::string signature;
        Location location;
        ByteCode code;
    };


    using ConstructionStack = std::stack<Construction>;
    using ConstructionList = std::vector<Construction>;


    struct CodeConstructor
    {
        InterpreterPtr interpreter;

        ConstructionStack stack;
        bool user_is_inserting_at_beginning = false;

        TokenList input_tokens;
        size_t current_token = 0;

        void compile_token(const Token& token);
        void compile_token_list();
    };



    using ConstructorStack = std::stack<CodeConstructor>;


}
