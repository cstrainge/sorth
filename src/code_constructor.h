
#pragma once


namespace sorth::internal
{


    struct Construction
    {
        bool is_immediate;

        std::string name;
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



    using OptionalConstructor = std::optional<CodeConstructor>;


}
